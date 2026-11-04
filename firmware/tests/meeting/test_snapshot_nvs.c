#include "meeting_snapshot_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t records[2][MEETING_SNAPSHOT_SIZE];static size_t lens[2];static int fail_commit;
static int index_of(const char *key){assert(strlen(key)==10);assert(!strncmp(key,"m1234abcd",9));return key[9]-'0';}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *len){(void)h;int i=index_of(key);if(!lens[i])return ESP_ERR_NVS_NOT_FOUND;if(*len<lens[i])return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,records[i],lens[i]);*len=lens[i];return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *p,size_t n){(void)h;int i=index_of(key);assert(n==MEETING_SNAPSHOT_SIZE);memcpy(records[i],p,n);lens[i]=n;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return fail_commit?99:ESP_OK;}
int main(void){meeting_snapshot_nvs n={.handle=1,.file_id=0x1234abcd};meeting_snapshot_io io;meeting_snapshot v={.generation=1,.valid_end=104,.frames=2,.reason=3},out;unsigned slot;memset(v.meeting_id,1,16);memset(v.source_sha256,2,32);assert(meeting_snapshot_nvs_bind(&n,&io));assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_EMPTY);assert(meeting_snapshot_commit(&io,&v));v.generation=2;fail_commit=1;assert(!meeting_snapshot_commit(&io,&v));fail_commit=0;lens[1]=3;assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_DEGRADED);assert(!meeting_snapshot_commit(&io,&v));n.file_id=0;assert(!meeting_snapshot_nvs_bind(&n,&io));return 0;}
