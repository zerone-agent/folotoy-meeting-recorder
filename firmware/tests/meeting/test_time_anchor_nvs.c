#include "meeting_time_anchor_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t blob[MEETING_TIME_ANCHOR_SIZE];static size_t length;static bool fail_commit;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *name,void *out,size_t *size){(void)h;assert(!strcmp(name,"anchor"));if(!length)return ESP_ERR_NVS_NOT_FOUND;if(*size<length)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,blob,length);*size=length;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *name,const void *data,size_t size){(void)h;assert(!strcmp(name,"anchor")&&size==sizeof blob);memcpy(blob,data,size);length=size;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return fail_commit?99:ESP_OK;}
static bool authorized(void *ctx,const meeting_time_anchor *a){(void)ctx;return a!=NULL;}
static bool key_ok(void *ctx,const meeting_time_public_key *key){(void)ctx;return key->public_key[0]==4&&key->public_key[1]==7;}
int main(void){
 nvs_handle_t handle=1;meeting_time_anchor_io io;meeting_time_anchor_session session={0};meeting_time_anchor a={0},out;
 a.origin_sha256[0]=1;a.key.public_key[0]=4;a.key.public_key[1]=7;assert(meeting_time_anchor_nvs_bind(&handle,&io));
 fail_commit=true;assert(meeting_time_anchor_install(&session,&io,&a,authorized,NULL,key_ok,NULL)==MEETING_ANCHOR_ERROR);
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ERROR);
 assert(meeting_time_anchor_install(&session,&io,&a,authorized,NULL,key_ok,NULL)==MEETING_ANCHOR_ERROR);
 fail_commit=false;assert(meeting_time_anchor_install(&session,&io,&a,authorized,NULL,key_ok,NULL)==MEETING_ANCHOR_OK);
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_OK);
 length=1;assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ERROR);
 return 0;
}
