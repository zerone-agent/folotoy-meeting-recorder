#include "meeting_file_sequence_nvs.h"
static int read_id(void *ctx,uint32_t *value)
{
    if(!ctx||!value)return -1;
    esp_err_t err=nvs_get_u32(*(nvs_handle_t *)ctx,"last_id",value);
    if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    return err==ESP_OK?0:-1;
}
static bool write_id(void *ctx,uint32_t value)
{
    if(!ctx)return false;
    nvs_handle_t handle=*(nvs_handle_t *)ctx;
    return nvs_set_u32(handle,"last_id",value)==ESP_OK&&nvs_commit(handle)==ESP_OK;
}
bool meeting_file_sequence_nvs_bind(nvs_handle_t *handle,meeting_file_sequence_io *io)
{
    if(!handle||!io)return false;
    *io=(meeting_file_sequence_io){.ctx=handle,.read=read_id,.write_sync=write_id};return true;
}
