#include "sdkconfig.h"
#if CONFIG_MEETING_DEVICE_TIME_PROBE
#include "meeting_commission.h"
#include "meeting_config_store_nvs.h"
#include "meeting_ca.h"
#include "meeting_sta.h"
#include "meeting_time_device.h"
#include "mbedtls/sha256.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include <stdlib.h>
#include <string.h>
static int64_t floor_value=1799999999LL;
static int read_floor(void *ctx,int64_t *out){(void)ctx;*out=floor_value;return 0;}
static bool commit_floor(void *ctx,int64_t value){(void)ctx;floor_value=value;return true;}
static bool set_time(void *ctx,int64_t value){(void)ctx;return value==1800000000LL;}
static int64_t probe_started;
static bool cancelled(void *ctx){(void)ctx;
#if CONFIG_MEETING_DEVICE_TIME_PROBE_CANCEL
 if(probe_started&&esp_timer_get_time()-probe_started>=1000000)return true;
#endif
 return !meeting_sta_has_ip();}
static int hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static void probe(void *unused){
 (void)unused;nvs_handle_t handle=0;meeting_config *config=calloc(1,sizeof(*config));
 uint8_t *scratch=malloc(MEETING_CONFIG_RECORD_MAX);meeting_config_store_io store;meeting_time_anchor anchor={0};
 const char *key=CONFIG_MEETING_DEVICE_TIME_PROBE_PUBLIC_KEY,*origin=CONFIG_MEETING_DEVICE_TIME_PROBE_HTTPS_ORIGIN;
 bool ok=config&&scratch&&strlen(key)==130&&nvs_open("meeting_config",NVS_READWRITE,&handle)==ESP_OK&&
    meeting_config_store_nvs_bind(&handle,&store)&&meeting_config_store_load(&store,meeting_ca_validate_pem,NULL,scratch,config)==MEETING_CONFIG_STORED;
 if(ok){for(size_t i=0;i<65;i++){int a=hex(key[2*i]),b=hex(key[2*i+1]);if(a<0||b<0){ok=false;break;}anchor.key.public_key[i]=(uint8_t)((a<<4)|b);}}
 if(ok)ok=mbedtls_sha256((const uint8_t *)origin,strlen(origin),anchor.origin_sha256,0)==0&&meeting_time_public_key_valid(NULL,&anchor.key);
 if(ok)ok=meeting_sta_start(config->ssid,config->password)==ESP_OK;
 if(ok){for(unsigned i=0;i<300&&!meeting_sta_has_ip();i++)vTaskDelay(pdMS_TO_TICKS(100));ok=meeting_sta_has_ip();}
 if(ok&&CONFIG_MEETING_DEVICE_TIME_PROBE_DNS[0])ok=meeting_sta_probe_dns(CONFIG_MEETING_DEVICE_TIME_PROBE_DNS)==ESP_OK;
 if(ok){meeting_clock_io io={NULL,read_floor,commit_floor,set_time};meeting_clock clock={0};meeting_time_challenge challenge={0};
   probe_started=esp_timer_get_time();meeting_time_client_result result=meeting_time_device_sync(&challenge,origin,CONFIG_MEETING_DEVICE_TIME_PROBE_HTTP_ORIGIN,&anchor,cancelled,NULL,&io,&clock);
#if CONFIG_MEETING_DEVICE_TIME_PROBE_CANCEL
   ok=result==MEETING_TIME_CLIENT_PAUSED&&!clock.trusted&&floor_value==1799999999LL;
#else
   ok=result==MEETING_TIME_CLIENT_OK&&clock.trusted&&floor_value==1800000000LL;
#endif
 }
 if(meeting_sta_running()&&meeting_sta_stop()!=ESP_OK){ESP_LOGE("meeting_time_probe","Wi-Fi stop failed");ok=false;}
#if CONFIG_MEETING_DEVICE_TIME_PROBE_CANCEL
 int64_t elapsed=esp_timer_get_time()-probe_started;
 ESP_LOGI("meeting_time_probe","Cancellation and Wi-Fi release took %lld ms",(long long)(elapsed/1000));
 if(!probe_started||elapsed>6500000LL)ok=false;
#endif
 ESP_LOGI("meeting_time_probe", "RESULT=%s",ok?"PASS":"FAIL");
 if(handle)nvs_close(handle);
 if(scratch){memset(scratch,0,MEETING_CONFIG_RECORD_MAX);free(scratch);}
 if(config){meeting_config_clear(config);free(config);}vTaskDelete(NULL);
}
bool meeting_time_probe_start(void){return meeting_commission_allowed()&&xTaskCreate(probe,"time_probe",16384,NULL,4,NULL)==pdPASS;}
#endif
