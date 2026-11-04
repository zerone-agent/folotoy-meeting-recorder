#include "meeting_config_store_nvs.h"
static int read_record(void *ctx,uint8_t *out,size_t capacity,size_t *size)
{
    if(!ctx||!out||!size)return -1;
    size_t n=capacity;
    esp_err_t err=nvs_get_blob(*(nvs_handle_t *)ctx,"active",out,&n);
    if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    if(err!=ESP_OK)return -1;
    *size=n;return 0;
}
static bool write_record(void *ctx,const uint8_t *data,size_t size)
{
    if(!ctx||!data)return false;
    nvs_handle_t h=*(nvs_handle_t *)ctx;
    return nvs_set_blob(h,"active",data,size)==ESP_OK&&nvs_commit(h)==ESP_OK;
}
bool meeting_config_store_nvs_bind(nvs_handle_t *handle,meeting_config_store_io *io)
{
    if(!handle||!io)return false;
    *io=(meeting_config_store_io){.ctx=handle,.read=read_record,.write_sync=write_record};return true;
}
