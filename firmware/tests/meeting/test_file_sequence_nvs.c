#include "meeting_file_sequence_nvs.h"
#include <assert.h>
#include <string.h>
static uint32_t saved;static bool present;static int commit_fails;
esp_err_t nvs_get_u32(nvs_handle_t h,const char *key,uint32_t *out)
{(void)h;assert(!strcmp(key,"last_id"));if(!present)return ESP_ERR_NVS_NOT_FOUND;*out=saved;return ESP_OK;}
esp_err_t nvs_set_u32(nvs_handle_t h,const char *key,uint32_t value)
{(void)h;assert(!strcmp(key,"last_id"));saved=value;present=true;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return commit_fails?99:ESP_OK;}
int main(void)
{
    nvs_handle_t h=1;meeting_file_sequence_io io;uint32_t id=0;
    assert(meeting_file_sequence_nvs_bind(&h,&io));
    commit_fails=1;assert(!meeting_file_sequence_next(&io,100,0,&id)&&id==0);
    commit_fails=0;assert(meeting_file_sequence_next(&io,100,0,&id)&&id==102);
    assert(meeting_file_sequence_next(&io,100,0,&id)&&id==103);
    return 0;
}
