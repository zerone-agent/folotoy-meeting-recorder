#include "meeting_config_store_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t record[MEETING_CONFIG_RECORD_MAX];static size_t used;static int failed;
esp_err_t nvs_get_blob(nvs_handle_t handle,const char *key,void *out,size_t *size)
{
    (void)handle;assert(!strcmp(key,"active"));if(!used)return ESP_ERR_NVS_NOT_FOUND;
    if(*size<used)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,record,used);*size=used;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle,const char *key,const void *data,size_t size)
{
    (void)handle;assert(!strcmp(key,"active")&&size<=sizeof(record));memcpy(record,data,size);used=size;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle){(void)handle;return failed?99:ESP_OK;}
int main(void)
{
    nvs_handle_t handle=1;meeting_config_store_io io;uint8_t out[8];size_t size=0;
    assert(meeting_config_store_nvs_bind(&handle,&io));assert(io.read(io.ctx,out,sizeof(out),&size)==1);
    const uint8_t data[]={1,2,3};failed=1;assert(!io.write_sync(io.ctx,data,sizeof(data)));
    failed=0;assert(io.write_sync(io.ctx,data,sizeof(data)));
    assert(io.read(io.ctx,out,sizeof(out),&size)==0&&size==3&&!memcmp(out,data,3));
    assert(io.read(io.ctx,out,2,&size)==-1);return 0;
}
