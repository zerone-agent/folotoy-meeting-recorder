#include "meeting_seal_nvs.h"
#include <stdio.h>
#include <string.h>
static bool key_for(uint32_t id,char key[10]){return id&&snprintf(key,10,"s%08lx",(unsigned long)id)==9;}
meeting_seal_status meeting_seal_nvs_read(nvs_handle_t nvs,uint32_t id,meeting_seal *out){
 char key[10];uint8_t raw[MEETING_SEAL_SIZE];size_t size=sizeof(raw);meeting_seal s;
 if(!out||!key_for(id,key))return MEETING_SEAL_ERROR;
 esp_err_t err=nvs_get_blob(nvs,key,raw,&size);if(err==ESP_ERR_NVS_NOT_FOUND)return MEETING_SEAL_MISSING;
 if(err!=ESP_OK||size!=sizeof(raw)||!meeting_seal_decode(raw,&s)||s.file_id!=id)return MEETING_SEAL_ERROR;
 *out=s;return MEETING_SEAL_OK;
}
bool meeting_seal_nvs_commit(nvs_handle_t nvs,const meeting_seal *seal){
 char key[10];uint8_t raw[MEETING_SEAL_SIZE],check[MEETING_SEAL_SIZE];meeting_seal existing;
 if(!seal||!key_for(seal->file_id,key)||!meeting_seal_encode(seal,raw))return false;
 if(nvs_commit(nvs)!=ESP_OK)return false; /* settle any prior ambiguous write */
 meeting_seal_status status=meeting_seal_nvs_read(nvs,seal->file_id,&existing);
 if(status==MEETING_SEAL_OK)return meeting_seal_encode(&existing,check)&&!memcmp(raw,check,sizeof(raw));
 if(status!=MEETING_SEAL_MISSING||nvs_set_blob(nvs,key,raw,sizeof(raw))!=ESP_OK||nvs_commit(nvs)!=ESP_OK)return false;
 return meeting_seal_nvs_read(nvs,seal->file_id,&existing)==MEETING_SEAL_OK&&meeting_seal_encode(&existing,check)&&!memcmp(raw,check,sizeof(raw));
}
