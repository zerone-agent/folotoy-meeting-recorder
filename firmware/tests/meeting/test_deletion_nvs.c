#include "meeting_deletion_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t records[2][MEETING_DELETION_SIZE];static size_t lengths[2];static int fail_commit,commit_count,fail_at;
static int index_of(const char *key){assert(strlen(key)==10&&!strncmp(key,"x1234abcd",9));return key[9]-'0';}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size){(void)h;int i=index_of(key);if(!lengths[i])return ESP_ERR_NVS_NOT_FOUND;if(*size<lengths[i])return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,records[i],lengths[i]);*size=lengths[i];return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size){(void)h;int i=index_of(key);assert(size==MEETING_DELETION_SIZE);memcpy(records[i],data,size);lengths[i]=size;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;commit_count++;return fail_commit||commit_count==fail_at?(esp_err_t)-1:ESP_OK;}
int main(void){
 meeting_deletion_nvs store={.handle=1,.file_id=0x1234abcd};meeting_deletion_io io;meeting_deletion v={.generation=1,.file_id=0x1234abcd,.meeting_id={1},.operation_id={2}},out;unsigned slot;
 assert(meeting_deletion_nvs_bind(&store,&io));assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_EMPTY);
 fail_commit=1;assert(!meeting_deletion_commit(&io,&v));assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_IO_ERROR);
 fail_commit=0;assert(meeting_deletion_commit(&io,&v));
 v.generation=2;v.local_done=true;commit_count=0;fail_at=3;assert(!meeting_deletion_commit(&io,&v));
 fail_at=0;fail_commit=1;
 assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_IO_ERROR);
 fail_commit=0;assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_OK&&out.generation==2&&out.local_done);
 v.file_id=0x11111111;v.generation=3;assert(!meeting_deletion_commit(&io,&v));
 assert(meeting_deletion_encode(&v,records[1]));assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_DAMAGED);
 lengths[1]=3;assert(meeting_deletion_load(&io,&out,&slot)==MEETING_DELETION_DAMAGED);
 store.file_id=0;assert(!meeting_deletion_nvs_bind(&store,&io));return 0;
}
