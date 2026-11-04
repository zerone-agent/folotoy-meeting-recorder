#include "meeting_seal_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t blob[MEETING_SEAL_SIZE];static size_t len;static int failure,commits,fail_at;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){(void)h;assert(!strcmp(key,"s0000004d"));if(!len)return ESP_ERR_NVS_NOT_FOUND;if(*n<len)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,blob,len);*n=len;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *in,size_t n){(void)h;assert(!strcmp(key,"s0000004d")&&n==sizeof(blob));memcpy(blob,in,n);len=n;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;commits++;return failure||commits==fail_at?99:ESP_OK;}
int main(void){meeting_seal s={.file_id=77,.meeting_id={1},.frames=1,.bytes=100,.reason=1},out;
 assert(meeting_seal_nvs_read(1,77,&out)==MEETING_SEAL_MISSING);failure=1;assert(!meeting_seal_nvs_commit(1,&s));assert(!len);
 failure=0;commits=0;fail_at=2;assert(!meeting_seal_nvs_commit(1,&s)&&len==MEETING_SEAL_SIZE);
 failure=1;fail_at=0;assert(!meeting_seal_nvs_commit(1,&s));
 failure=0;assert(meeting_seal_nvs_commit(1,&s));assert(meeting_seal_nvs_read(1,77,&out)==MEETING_SEAL_OK&&out.frames==1);
 assert(meeting_seal_nvs_commit(1,&s));s.frames=2;assert(!meeting_seal_nvs_commit(1,&s));blob[7]^=1;
 assert(meeting_seal_nvs_read(1,77,&out)==MEETING_SEAL_ERROR);assert(!meeting_seal_nvs_commit(1,&s));return 0;}
