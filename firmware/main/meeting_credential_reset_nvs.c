#include "meeting_credential_reset_nvs.h"

static int read_record(void *ctx,uint8_t out[MEETING_RESET_RECORD_SIZE])
{
    meeting_reset_nvs *s=ctx;size_t length=MEETING_RESET_RECORD_SIZE;
    if(!s||!out)return -1;
    esp_err_t err=nvs_get_blob(s->handle,"pending",out,&length);
    if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    return err==ESP_OK&&length==MEETING_RESET_RECORD_SIZE?0:-1;
}
static bool write_record(void *ctx,const uint8_t data[MEETING_RESET_RECORD_SIZE])
{
    meeting_reset_nvs *s=ctx;
    return s&&data&&nvs_set_blob(s->handle,"pending",data,MEETING_RESET_RECORD_SIZE)==ESP_OK&&nvs_commit(s->handle)==ESP_OK;
}
static int credential_state(void *ctx,const meeting_reset_record *record)
{
    meeting_reset_nvs *s=ctx;return s->credential_state(s->credential_ctx,record);
}
static bool clear_credentials(void *ctx)
{
    meeting_reset_nvs *s=ctx;return s->clear_credentials(s->credential_ctx);
}
bool meeting_reset_nvs_bind(meeting_reset_nvs *state,meeting_reset_io *out)
{
    if(!state||!out||!state->credential_state||!state->clear_credentials)return false;
    *out=(meeting_reset_io){state,read_record,write_record,credential_state,clear_credentials};return true;
}
