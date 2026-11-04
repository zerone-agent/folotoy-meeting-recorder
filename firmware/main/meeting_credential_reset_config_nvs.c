#include "meeting_credential_reset_config_nvs.h"
#include <stdlib.h>
#include <string.h>

int meeting_reset_config_state(void *ctx,const meeting_reset_record *record)
{
    meeting_reset_config_nvs *s=ctx;
    if(!s||!record||!s->validate_ca)return -1;
    uint8_t *scratch=malloc(MEETING_CONFIG_RECORD_MAX);
    meeting_config *config=calloc(1,sizeof(*config));
    if(!scratch||!config){free(scratch);free(config);return -1;}
    meeting_config_store_io io;
    int result=-1;
    if(meeting_config_store_nvs_bind(&s->config_handle,&io)){
        meeting_config_store_status status=meeting_config_store_load(&io,s->validate_ca,s->ca_ctx,scratch,config);
        if(status==MEETING_CONFIG_ABSENT)result=1;
        else if(status==MEETING_CONFIG_STORED){
            meeting_reset_record expected;
            if(meeting_reset_identity_from_config(config,record->event_id,&expected)&&
               !strcmp(expected.origin,record->origin)&&!strcmp(expected.device_id,record->device_id)&&
               !strcmp(expected.credential_id,record->credential_id))result=0;
        }
    }
    volatile uint8_t *wipe=scratch;for(size_t i=0;i<MEETING_CONFIG_RECORD_MAX;i++)wipe[i]=0;
    free(scratch);meeting_config_clear(config);free(config);return result;
}
bool meeting_reset_config_clear(void *ctx)
{
    meeting_reset_config_nvs *s=ctx;if(!s)return false;
    esp_err_t err=nvs_erase_key(s->config_handle,"active");
    if(err!=ESP_OK&&err!=ESP_ERR_NVS_NOT_FOUND)return false;
    /* Recommit even when already absent: a previous erase may only have been
     * staged when its commit failed. The state machine reads back afterwards. */
    return nvs_commit(s->config_handle)==ESP_OK;
}
