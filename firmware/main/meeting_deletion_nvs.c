#include "meeting_deletion_nvs.h"
#include <stdio.h>
#include <string.h>
static bool key_for(const meeting_deletion_nvs *s,unsigned slot,char key[12]){
 return s&&s->file_id&&slot<2&&snprintf(key,12,"x%08lx%u",(unsigned long)s->file_id,slot)==10;
}
static int read_slot(void *ctx,unsigned slot,uint8_t out[MEETING_DELETION_SIZE]){
 meeting_deletion_nvs *s=ctx;char key[12];size_t size=MEETING_DELETION_SIZE;
 if(!out||!key_for(s,slot,key))return -1;
 /* An earlier failed commit can leave a staged value visible. Recommit before
  * treating either presence or absence as durable evidence. */
 if(nvs_commit(s->handle)!=ESP_OK)return -1;
 esp_err_t err=nvs_get_blob(s->handle,key,out,&size);
 if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
 if(err==ESP_ERR_NVS_INVALID_LENGTH||(err==ESP_OK&&size!=MEETING_DELETION_SIZE)){memset(out,0,MEETING_DELETION_SIZE);return 0;}
 if(err==ESP_OK){meeting_deletion value;if(meeting_deletion_decode(out,&value)&&value.file_id!=s->file_id)memset(out,0,MEETING_DELETION_SIZE);}
 return err==ESP_OK?0:-1;
}
static bool write_slot(void *ctx,unsigned slot,const uint8_t data[MEETING_DELETION_SIZE]){
 meeting_deletion_nvs *s=ctx;char key[12];meeting_deletion value;
 return data&&key_for(s,slot,key)&&meeting_deletion_decode(data,&value)&&value.file_id==s->file_id&&nvs_set_blob(s->handle,key,data,MEETING_DELETION_SIZE)==ESP_OK&&nvs_commit(s->handle)==ESP_OK;
}
bool meeting_deletion_nvs_bind(meeting_deletion_nvs *s,meeting_deletion_io *io){
 if(!s||!io||!s->file_id)return false;
 *io=(meeting_deletion_io){.ctx=s,.read=read_slot,.write_sync=write_slot};return true;
}
