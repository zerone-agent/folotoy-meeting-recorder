#include "meeting_deletion_store.h"
#include <assert.h>
#include <string.h>
static int opened,fail_open,iterator_count,iterator_error,invalid_type,released,namespace_missing,present_slot=-1,read_error;
static const char *keys[4];struct fake_nvs_iterator {int index;};static struct fake_nvs_iterator cursor;
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *handle){assert(!strcmp(name,"meeting_delete")&&(mode==NVS_READWRITE||mode==NVS_READONLY));if(fail_open)return 99;if(namespace_missing&&mode==NVS_READONLY)return ESP_ERR_NVS_NOT_FOUND;opened++;*handle=42;return ESP_OK;}
void nvs_close(nvs_handle_t h){assert(h==42);opened--;}
esp_err_t nvs_commit(nvs_handle_t h){assert(h==42);return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size){(void)out;(void)size;assert(h==42&&!strncmp(key,"x00000007",9)&&(key[9]=='0'||key[9]=='1')&&!key[10]);if(read_error)return 99;if(present_slot==key[9]-'0')return ESP_OK;return ESP_ERR_NVS_NOT_FOUND;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size){(void)h;(void)key;(void)data;(void)size;return 99;}
esp_err_t nvs_entry_find(const char *partition,const char *name,int type,nvs_iterator_t *it){assert(!strcmp(partition,"nvs")&&!strcmp(name,"meeting_delete")&&type==NVS_TYPE_ANY);if(iterator_error)return 99;if(!iterator_count){*it=NULL;return ESP_ERR_NVS_NOT_FOUND;}cursor.index=0;*it=&cursor;return ESP_OK;}
esp_err_t nvs_entry_next(nvs_iterator_t *it){if(++cursor.index>=iterator_count){*it=NULL;return ESP_ERR_NVS_NOT_FOUND;}return ESP_OK;}
esp_err_t nvs_entry_info(nvs_iterator_t it,nvs_entry_info_t *info){assert(it==&cursor);memset(info,0,sizeof(*info));strcpy(info->key,keys[cursor.index]);info->type=invalid_type?NVS_TYPE_ANY:NVS_TYPE_BLOB;return ESP_OK;}
void nvs_release_iterator(nvs_iterator_t it){assert(it==&cursor);released++;}
int main(void){meeting_deletion_store store;meeting_deletion_io io;meeting_deletion state;unsigned slot;
 assert(!meeting_deletion_store_open(&store,0,&io));fail_open=1;assert(!meeting_deletion_store_open(&store,7,&io)&&!opened);
 fail_open=0;assert(meeting_deletion_store_open(&store,7,&io)&&opened==1);
 assert(meeting_deletion_load(&io,&state,&slot)==MEETING_DELETION_EMPTY);meeting_deletion_store_close(&store);assert(!opened);
 assert(meeting_deletion_load(&io,&state,&slot)==MEETING_DELETION_IO_ERROR);meeting_deletion_store_close(&store);
 uint32_t id;assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_END);
 keys[0]="x0000000a1";keys[1]="x000000070";keys[2]="x000000071";iterator_count=3;
 assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_FOUND&&id==7);
 assert(meeting_deletion_store_next(7,&id)==MEETING_DELETION_NEXT_FOUND&&id==10);
 assert(meeting_deletion_store_next(10,&id)==MEETING_DELETION_NEXT_END);
 keys[1]="unexpected";assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_ERROR&&released==1);
 keys[1]="x000000070";invalid_type=1;assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_ERROR&&released==2);invalid_type=0;
 keys[1]="x000000000";assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_ERROR&&released==3);
 iterator_error=1;assert(meeting_deletion_store_next(0,&id)==MEETING_DELETION_NEXT_ERROR);
 namespace_missing=1;assert(meeting_deletion_store_upload_permitted(7));namespace_missing=0;
 assert(meeting_deletion_store_upload_permitted(7));present_slot=1;assert(!meeting_deletion_store_upload_permitted(7));present_slot=-1;
 read_error=1;assert(!meeting_deletion_store_upload_permitted(7));read_error=0;fail_open=1;assert(!meeting_deletion_store_upload_permitted(7));
 assert(!meeting_deletion_store_upload_permitted(0)&&!opened);return 0;}
