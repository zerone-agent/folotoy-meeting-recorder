#include "meeting_confirmation_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t saved[MEETING_CONFIRMATION_SIZE];
static size_t saved_len;static int fail_commit;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *len) {
    (void)h;assert(!strcmp(key,"c1234abcd"));if(!saved_len)return ESP_ERR_NVS_NOT_FOUND;
    if(*len<saved_len)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,saved,saved_len);*len=saved_len;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t len) {
    (void)h;assert(!strcmp(key,"c1234abcd")&&len==sizeof(saved));memcpy(saved,data,len);saved_len=len;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {(void)h;return fail_commit?99:ESP_OK;}
int main(void) {
    meeting_confirmation_nvs n={.handle=1,.file_id=0x1234abcd};meeting_confirmation_io io;
    meeting_confirmation value={.file_id=0x1234abcd,.source_sha256={1},.manifest={.count=1,.bytes=100,.digest={2}}},out;
    memcpy(value.manifest.id,"12345678-1234-4123-8123-123456789abc",37);
    assert(meeting_confirmation_nvs_bind(&n,&io));assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_EMPTY);
    fail_commit=1;assert(!meeting_confirmation_commit(&io,&value));fail_commit=0;
    assert(meeting_confirmation_commit(&io,&value));
    assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_OK);
    saved_len=3;assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);
    assert(!meeting_confirmation_commit(&io,&value));n.file_id=0;assert(!meeting_confirmation_nvs_bind(&n,&io));return 0;
}
