#include "meeting_retirement_nvs.h"
#include <assert.h>
#include <string.h>
typedef struct {int handle;char key[12];uint8_t data[MEETING_RETIRED_SIZE];size_t size;} entry;
static entry entries[8];static int fail_commit;
static entry *find(int h,const char *key){for(int i=0;i<8;i++)if(entries[i].handle==h&&!strcmp(entries[i].key,key))return &entries[i];return NULL;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *len)
{
    entry *e=find(h,key);if(!e)return ESP_ERR_NVS_NOT_FOUND;if(!out){*len=e->size;return ESP_OK;}
    if(*len<e->size)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,e->data,e->size);*len=e->size;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size)
{
    entry *e=find(h,key);if(!e)for(int i=0;i<8;i++)if(!entries[i].handle){e=&entries[i];e->handle=h;strcpy(e->key,key);break;}
    assert(e&&size<=sizeof(e->data));memcpy(e->data,data,size);e->size=size;return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t h,const char *key){entry *e=find(h,key);if(!e)return ESP_ERR_NVS_NOT_FOUND;memset(e,0,sizeof(*e));return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return fail_commit?99:ESP_OK;}
static bool no_cancel(void *ctx){(void)ctx;return false;}
static bool proof(void *ctx,const meeting_retired *id){(void)ctx;return id->file_id==7;}
static bool absent(void *ctx){(void)ctx;return true;}
int main(void)
{
    uint8_t dummy=1;assert(nvs_set_blob(2,"m000000070",&dummy,1)==ESP_OK);assert(nvs_set_blob(3,"c00000007",&dummy,1)==ESP_OK);
    assert(nvs_set_blob(4,"r00000007",&dummy,1)==ESP_OK);assert(nvs_set_blob(4,"s00000007",&dummy,1)==ESP_OK);assert(nvs_set_blob(5,"c00000007",&dummy,1)==ESP_OK);
    meeting_retirement_nvs state={.markers=1,.snapshots=2,.receipts=3,.records=4,.intents=5,.file_id=7,
        .cancelled=no_cancel,.verify_completed=proof,.files_absent=absent};meeting_retirement_io io;
    meeting_retired id={.file_id=7,.meeting_id={1}};assert(meeting_retirement_nvs_bind(&state,&io));
    fail_commit=1;assert(meeting_retirement_run(&id,true,&io)==MEETING_RETIRE_ERROR);
    assert(find(2,"m000000070")&&find(3,"c00000007")&&find(4,"r00000007")&&find(4,"s00000007")&&find(5,"c00000007"));
    fail_commit=0;assert(meeting_retirement_run(&id,true,&io)==MEETING_RETIRE_DONE);
    assert(find(1,"d00000007")&&!find(2,"m000000070")&&!find(3,"c00000007")&&!find(4,"r00000007")&&!find(4,"s00000007")&&!find(5,"c00000007"));
    meeting_retired decoded;assert(meeting_retirement_nvs_read(1,7,&decoded)==0&&decoded.file_id==7);
    assert(meeting_retirement_nvs_read(1,8,&decoded)==1);
    assert(meeting_retirement_run(&id,false,&io)==MEETING_RETIRE_DONE);
    find(1,"d00000007")->data[5]^=1;assert(meeting_retirement_nvs_read(1,7,&decoded)==-1);
    return 0;
}
