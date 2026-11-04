#include "meeting_deletion_store.h"
#include <string.h>
#include <stdio.h>
bool meeting_deletion_store_open(meeting_deletion_store *s,uint32_t id,meeting_deletion_io *io){
 if(!s||!io||!id)return false;
 *s=(meeting_deletion_store){0};
 if(nvs_open("meeting_delete",NVS_READWRITE,&s->nvs.handle)!=ESP_OK)return false;
 s->opened=true;s->nvs.file_id=id;
 if(!meeting_deletion_nvs_bind(&s->nvs,io)){meeting_deletion_store_close(s);return false;}
 return true;
}
void meeting_deletion_store_close(meeting_deletion_store *s){
 if(s&&s->opened){nvs_close(s->nvs.handle);s->opened=false;s->nvs.file_id=0;}
}
static bool parse_key(const char *key,uint32_t *id){
 if(!key||!id||strlen(key)!=10||key[0]!='x'||(key[9]!='0'&&key[9]!='1'))return false;
 uint32_t value=0;for(unsigned i=1;i<9;i++){
  char c=key[i];unsigned n;if(c>='0'&&c<='9')n=(unsigned)(c-'0');else if(c>='a'&&c<='f')n=(unsigned)(c-'a'+10);else return false;
  value=(value<<4)|n;
 }
 if(!value)return false;
 *id=value;return true;
}
meeting_deletion_next_status meeting_deletion_store_next(uint32_t after,uint32_t *file_id){
 if(!file_id)return MEETING_DELETION_NEXT_ERROR;
 nvs_iterator_t iterator=NULL;uint32_t next=0;bool found=false;esp_err_t err=nvs_entry_find("nvs","meeting_delete",NVS_TYPE_ANY,&iterator);
 while(err==ESP_OK){
  nvs_entry_info_t entry;uint32_t id;
  if(nvs_entry_info(iterator,&entry)!=ESP_OK||entry.type!=NVS_TYPE_BLOB||!parse_key(entry.key,&id)){err=-1;break;}
  if(id>after&&(!found||id<next)){next=id;found=true;}
  err=nvs_entry_next(&iterator);
 }
 if(iterator)nvs_release_iterator(iterator);
 if(err!=ESP_ERR_NVS_NOT_FOUND)return MEETING_DELETION_NEXT_ERROR;
 if(!found)return MEETING_DELETION_NEXT_END;
 *file_id=next;return MEETING_DELETION_NEXT_FOUND;
}
bool meeting_deletion_store_upload_permitted(uint32_t file_id){
 if(!file_id)return false;
 nvs_handle_t handle;esp_err_t err=nvs_open("meeting_delete",NVS_READONLY,&handle);
 if(err==ESP_ERR_NVS_NOT_FOUND)return true; /* namespace has never been created */
 if(err!=ESP_OK)return false;
 bool clear=true;
 for(unsigned slot=0;slot<2;slot++){
  char key[12];size_t size=0;
  if(snprintf(key,sizeof(key),"x%08lx%u",(unsigned long)file_id,slot)!=10){clear=false;break;}
  err=nvs_get_blob(handle,key,NULL,&size);
  if(err!=ESP_ERR_NVS_NOT_FOUND){clear=false;break;}
 }
 nvs_close(handle);return clear;
}
