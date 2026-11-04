#include "meeting_clock_nvs.h"
#include <assert.h>
#include <string.h>
static unsigned char saved[16];static size_t saved_len;static int commit_fail;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *len)
{
    (void)h;assert(!strcmp(key,"floor"));if(!saved_len)return ESP_ERR_NVS_NOT_FOUND;
    if(*len<saved_len)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,saved,saved_len);*len=saved_len;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t len)
{
    (void)h;assert(!strcmp(key,"floor")&&len==sizeof(saved));memcpy(saved,data,len);saved_len=len;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return commit_fail?99:ESP_OK;}
int main(void)
{
    nvs_handle_t handle=1;meeting_clock_io io;int64_t floor=0;
    assert(meeting_clock_nvs_bind(&handle,&io));assert(io.read_floor(io.ctx,&floor)==1);
    assert(io.commit_floor(io.ctx,1800000000));assert(io.read_floor(io.ctx,&floor)==0&&floor==1800000000);
    assert(!io.commit_floor(io.ctx,1799999999));
    commit_fail=1;assert(!io.commit_floor(io.ctx,1800000001));commit_fail=0;
    saved[5]^=1;assert(io.read_floor(io.ctx,&floor)==-1);assert(!io.commit_floor(io.ctx,1800000002));
    return 0;
}
