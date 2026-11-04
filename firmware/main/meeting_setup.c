#include "meeting_setup.h"
#include "meeting_ca.h"
#include "meeting_commission.h"
#include "meeting_clock_nvs.h"
#include "meeting_config_ble.h"
#include "meeting_config_store_nvs.h"
#include "meeting_network.h"
#include "meeting_pairing_ble.h"
#include "meeting_provision_apply.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs.h"
#include <stdlib.h>
#include <string.h>

/* All state, including NVS handles referenced by the network worker, has
 * static lifetime. The BLE inbox itself synchronizes with the host task. */
static meeting_config_ble_inbox *inbox;
static bool handles_ready;
static nvs_handle_t clock_handle, config_handle;
static meeting_clock_io clock_io;
static meeting_config_store_io store_io;
static meeting_setup_view current;
static meeting_pairing_token token;
static uint8_t *json;
static size_t json_length;
static int64_t deadline;
static uint64_t received_uptime_ms;
static meeting_config *pending_config;
static meeting_clock pending_clock;
static meeting_setup_result last_result;
static const char *TAG="meeting_setup";

static void wipe(void)
{
    volatile uint8_t *p=json;
    if(p){for(size_t i=0;i<MEETING_CONFIG_MAX_BYTES;i++)p[i]=0;free(json);json=NULL;}
    json_length=0;memset(&token,0,sizeof(token));
    if(pending_config){meeting_config_clear(pending_config);free(pending_config);pending_config=NULL;}
    meeting_clock_reset(&pending_clock);
}
static bool session_valid(void *ctx)
{return meeting_pairing_ble_session_valid(*(meeting_pairing_token *)ctx);}

esp_err_t meeting_setup_start(void)
{
    if(current.state!=MEETING_SETUP_IDLE||meeting_network_enabled())return ESP_ERR_INVALID_STATE;
    last_result=MEETING_SETUP_NO_RESULT;
    if(!handles_ready){
        esp_err_t err=nvs_open("meeting_clock",NVS_READWRITE,&clock_handle);
        if(err!=ESP_OK)return err;
        err=nvs_open("meeting_config",NVS_READWRITE,&config_handle);
        if(err!=ESP_OK){nvs_close(clock_handle);return err;}
        if(!meeting_clock_nvs_bind(&clock_handle,&clock_io)||!meeting_config_store_nvs_bind(&config_handle,&store_io)){
            nvs_close(clock_handle);nvs_close(config_handle);return ESP_FAIL;
        }
        handles_ready=true;
    }
    wipe();memset(&current,0,sizeof(current));
    inbox=calloc(1,sizeof(*inbox));json=calloc(1,MEETING_CONFIG_MAX_BYTES);
    if(!inbox||!json){free(inbox);inbox=NULL;wipe();return ESP_ERR_NO_MEM;}
    if(!meeting_config_ble_init(inbox)){free(inbox);inbox=NULL;wipe();return ESP_ERR_NO_MEM;}
    meeting_config_ble_prepare(inbox);
    esp_err_t err=meeting_pairing_ble_start(current.code,meeting_config_ble_handle,meeting_config_ble_reset,inbox);
    if(err!=ESP_OK){
        /* A failed start can leave a transport alive if its rollback failed.
         * Do not release its callback context until stop succeeds. */
        if(meeting_pairing_ble_stop()!=ESP_OK){
            wipe();current.state=MEETING_SETUP_PAIRING;
            deadline=esp_timer_get_time()+120LL*1000000LL;
            return err;
        }
        meeting_config_ble_destroy(inbox);free(inbox);inbox=NULL;wipe();memset(&current,0,sizeof(current));return err;
    }
    deadline=esp_timer_get_time()+120LL*1000000LL;
    current.state=MEETING_SETUP_PAIRING;return ESP_OK;
}

void meeting_setup_poll(void)
{
    if(current.state==MEETING_SETUP_IDLE)return;
    if(current.state==MEETING_SETUP_SAVED){
        /* The configuration and clock were already committed while the
         * authenticated session was valid. A lost BLE acknowledgement must
         * not prevent networking. Keep the context alive if stop fails. */
        if(meeting_config_ble_acknowledged(inbox,token)||!session_valid(&token)||esp_timer_get_time()>=deadline){
            esp_err_t err=meeting_pairing_ble_stop();
            if(err==ESP_OK){
                meeting_config_ble_destroy(inbox);free(inbox);inbox=NULL;
                esp_err_t network_result=meeting_network_activate(pending_config,&pending_clock,&clock_io);
                if(network_result!=ESP_OK)ESP_LOGE(TAG,"Network activation failed: %s",esp_err_to_name(network_result));
                last_result=network_result==ESP_OK?MEETING_SETUP_NETWORK_STARTED:MEETING_SETUP_NETWORK_FAILED;
                wipe();memset(&current,0,sizeof(current));
            }
        }
        return;
    }
    if(esp_timer_get_time()>=deadline){(void)meeting_setup_cancel();return;}
    if(current.state==MEETING_SETUP_REVIEW||current.state==MEETING_SETUP_FIRST_CLOCK){
        if(!session_valid(&token))(void)meeting_setup_cancel();
        return;
    }
    if(!meeting_config_ble_take(inbox,json,MEETING_CONFIG_MAX_BYTES,&json_length,&token))return;
    meeting_config *parsed=calloc(1,sizeof(*parsed));
    if(!parsed){wipe();(void)meeting_setup_cancel();return;}
    bool ok=meeting_config_parse(json,json_length,meeting_ca_validate_pem,NULL,parsed)&&session_valid(&token);
    if(ok){memcpy(current.ca_sha256,parsed->ca_sha256,32);current.utc=parsed->utc;received_uptime_ms=(uint64_t)esp_timer_get_time()/1000;current.state=MEETING_SETUP_REVIEW;}
    meeting_config_clear(parsed);free(parsed);
    if(!ok){wipe();(void)meeting_setup_cancel();}
}

void meeting_setup_view_get(meeting_setup_view *view){if(view)*view=current;}
meeting_setup_result meeting_setup_take_result(void)
{meeting_setup_result result=last_result;last_result=MEETING_SETUP_NO_RESULT;return result;}

esp_err_t meeting_setup_confirm(void)
{
    if((current.state!=MEETING_SETUP_REVIEW&&current.state!=MEETING_SETUP_FIRST_CLOCK)||!session_valid(&token))return ESP_ERR_INVALID_STATE;
    int64_t existing_floor=0;
    if(!clock_io.read_floor)return MEETING_SETUP_ERR_CLOCK;
    int floor_status=clock_io.read_floor(clock_io.ctx,&existing_floor);
    if(floor_status!=0&&floor_status!=1)return MEETING_SETUP_ERR_CLOCK;
    if(floor_status==1&&!meeting_commission_allowed())return MEETING_SETUP_ERR_CLOCK;
    if(floor_status==1&&current.state==MEETING_SETUP_REVIEW){
        /* A second, explicit on-device confirmation is required. Absence of
         * NVS is not evidence of a factory state or permission to format. */
        current.state=MEETING_SETUP_FIRST_CLOCK;return ESP_OK;
    }
    bool initial_floor=floor_status==1&&current.state==MEETING_SETUP_FIRST_CLOCK;
    uint8_t *scratch=malloc(MEETING_CONFIG_RECORD_MAX);
    pending_config=calloc(1,sizeof(*pending_config));
    meeting_config *config=pending_config;
    if(!scratch||!config){free(scratch);free(pending_config);pending_config=NULL;ESP_LOGE(TAG,"confirmation allocation failed");return ESP_ERR_NO_MEM;}
    meeting_clock clock;
    /* Supervised prototype commissioning: physical approval permits a new
     * time floor, not storage initialization or a tamper-resistance claim. */
    meeting_provision_result result=meeting_provision_apply(json,json_length,current.ca_sha256,current.utc,true,
        session_valid,&token,initial_floor,received_uptime_ms,(uint64_t)esp_timer_get_time()/1000,&clock_io,&clock,&store_io,
        meeting_ca_validate_pem,NULL,scratch,config);
    if(result!=MEETING_PROVISION_APPLIED)ESP_LOGE(TAG,"confirmation result=%d",(int)result);
    esp_err_t err=result==MEETING_PROVISION_CLOCK_FAILED?MEETING_SETUP_ERR_CLOCK:ESP_FAIL;
    if(result==MEETING_PROVISION_APPLIED){
        /* Publishing is only a notification. The authenticated apply has
         * already committed configuration, even if BLE disconnected before
         * this notification could be queued. */
        (void)meeting_config_ble_publish(inbox,token);
        pending_clock=clock;
        volatile uint8_t *p=json;for(size_t i=0;i<MEETING_CONFIG_MAX_BYTES;i++)p[i]=0;
        json_length=0;current.state=MEETING_SETUP_SAVED;err=ESP_OK;
    }
    memset(scratch,0,MEETING_CONFIG_RECORD_MAX);free(scratch);
    if(err!=ESP_OK){
        wipe();
        /* A failed stop retains transport ownership for a later retry. */
        current.state=MEETING_SETUP_PAIRING;
        if(meeting_pairing_ble_stop()!=ESP_OK)return err;
    }
    if(err!=ESP_OK){meeting_config_ble_destroy(inbox);free(inbox);inbox=NULL;memset(&current,0,sizeof(current));}
    return err;
}

esp_err_t meeting_setup_cancel(void)
{
    if(current.state==MEETING_SETUP_IDLE)return ESP_OK;
    esp_err_t err=meeting_pairing_ble_stop();
    wipe();
    if(err==ESP_OK){meeting_config_ble_destroy(inbox);free(inbox);inbox=NULL;memset(&current,0,sizeof(current));}
    else current.state=MEETING_SETUP_PAIRING;
    return err;
}
