#include "meeting_delete_proof_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t blob[MEETING_CONFIRMATION_SIZE];static size_t length;static int fail,opened,commits,fail_at;
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){assert(!strcmp(name,"meeting_dproof")&&mode==NVS_READWRITE);opened++;*h=1;return ESP_OK;}
void nvs_close(nvs_handle_t h){assert(h==1);opened--;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){assert(h==1&&!strcmp(key,"p00000007"));if(!length)return ESP_ERR_NVS_NOT_FOUND;if(*n<length)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,blob,length);*n=length;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *in,size_t n){assert(h==1&&!strcmp(key,"p00000007")&&n==sizeof(blob));memcpy(blob,in,n);length=n;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){assert(h==1);commits++;return fail||commits==fail_at?99:ESP_OK;}
int main(void){
 meeting_delete_proof_nvs storage;meeting_confirmation_io io;meeting_confirmation proof={.file_id=7,.manifest={.count=1,.bytes=100,.reason=0}};
 memcpy(proof.manifest.id,"12345678-1234-4123-8123-123456789abc",37);memset(proof.manifest.digest,1,32);memset(proof.source_sha256,2,32);
 assert(meeting_delete_proof_nvs_open(&storage,7,&io)&&opened==1);
 meeting_confirmation out;assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_EMPTY);
 fail=1;assert(!meeting_confirmation_commit(&io,&proof)&&!length);fail=0;
 commits=0;fail_at=2;assert(!meeting_confirmation_commit(&io,&proof)&&length==MEETING_CONFIRMATION_SIZE);
 fail_at=0;fail=1;assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);fail=0;
 assert(meeting_confirmation_commit(&io,&proof)&&meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_OK&&out.file_id==7);
 proof.file_id=8;assert(!meeting_confirmation_commit(&io,&proof));blob[0]^=1;assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);
 meeting_delete_proof_nvs_close(&storage);assert(!opened&&meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);return 0;
}
