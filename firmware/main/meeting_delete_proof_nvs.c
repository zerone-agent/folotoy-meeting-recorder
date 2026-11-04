#include "meeting_delete_proof_nvs.h"
#include <stdio.h>
#include <string.h>
static bool key_for(const meeting_delete_proof_nvs *s,char key[10]){
 return s&&s->opened&&s->file_id&&snprintf(key,10,"p%08lx",(unsigned long)s->file_id)==9;
}
static int read_slot(void *ctx,uint8_t out[MEETING_CONFIRMATION_SIZE]){
 meeting_delete_proof_nvs *s=ctx;char key[10];size_t size=MEETING_CONFIRMATION_SIZE;
 if(!out||!key_for(s,key)||nvs_commit(s->handle)!=ESP_OK)return -1;
 esp_err_t err=nvs_get_blob(s->handle,key,out,&size);if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
 if(err==ESP_ERR_NVS_INVALID_LENGTH||(err==ESP_OK&&size!=MEETING_CONFIRMATION_SIZE)){memset(out,0,MEETING_CONFIRMATION_SIZE);return 0;}
 return err==ESP_OK?0:-1;
}
static bool write_slot(void *ctx,const uint8_t data[MEETING_CONFIRMATION_SIZE]){
 meeting_delete_proof_nvs *s=ctx;char key[10];meeting_confirmation proof;
 return data&&key_for(s,key)&&meeting_confirmation_decode(data,&proof)&&proof.file_id==s->file_id&&
        nvs_set_blob(s->handle,key,data,MEETING_CONFIRMATION_SIZE)==ESP_OK&&nvs_commit(s->handle)==ESP_OK;
}
bool meeting_delete_proof_nvs_open(meeting_delete_proof_nvs *s,uint32_t id,meeting_confirmation_io *io){
 if(!s||!io||!id)return false;
 *s=(meeting_delete_proof_nvs){.file_id=id};
 if(nvs_open("meeting_dproof",NVS_READWRITE,&s->handle)!=ESP_OK)return false;
 s->opened=true;*io=(meeting_confirmation_io){.ctx=s,.read=read_slot,.write_sync=write_slot};return true;
}
void meeting_delete_proof_nvs_close(meeting_delete_proof_nvs *s){if(s&&s->opened){nvs_close(s->handle);s->opened=false;}}
