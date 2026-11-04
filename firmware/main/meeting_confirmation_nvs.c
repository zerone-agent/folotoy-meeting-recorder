#include "meeting_confirmation_nvs.h"
#include <stdio.h>
#include <string.h>

static bool key_for(const meeting_confirmation_nvs *s,char key[10])
{
    return s && s->file_id && snprintf(key,10,"c%08lx",(unsigned long)s->file_id)==9;
}
static int read_slot(void *ctx,uint8_t out[MEETING_CONFIRMATION_SIZE])
{
    meeting_confirmation_nvs *s=ctx;char key[10];size_t n=MEETING_CONFIRMATION_SIZE;
    if(!out||!key_for(s,key))return -1;
    esp_err_t err=nvs_get_blob(s->handle,key,out,&n);
    if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    if(err==ESP_ERR_NVS_INVALID_LENGTH||(err==ESP_OK&&n!=MEETING_CONFIRMATION_SIZE)){
        memset(out,0,MEETING_CONFIRMATION_SIZE);return 0;
    }
    return err==ESP_OK?0:-1;
}
static bool write_slot(void *ctx,const uint8_t data[MEETING_CONFIRMATION_SIZE])
{
    meeting_confirmation_nvs *s=ctx;char key[10];
    return data&&key_for(s,key)&&nvs_set_blob(s->handle,key,data,MEETING_CONFIRMATION_SIZE)==ESP_OK&&nvs_commit(s->handle)==ESP_OK;
}
bool meeting_confirmation_nvs_bind(meeting_confirmation_nvs *storage,meeting_confirmation_io *io)
{
    if(!storage||!storage->file_id||!io)return false;
    *io=(meeting_confirmation_io){.ctx=storage,.read=read_slot,.write_sync=write_slot};return true;
}
