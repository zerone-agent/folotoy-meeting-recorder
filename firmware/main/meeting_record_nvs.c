#include "meeting_record_nvs.h"
#include <stdio.h>
#include <string.h>
static bool key_for(uint32_t id,char key[10]){return id&&snprintf(key,10,"r%08lx",(unsigned long)id)==9;}
meeting_record_status meeting_record_nvs_read(nvs_handle_t nvs,uint32_t file_id,meeting_record *out)
{
 char key[10];uint8_t bytes[MEETING_RECORD_SIZE];size_t n=sizeof(bytes);meeting_record decoded;
 if(!out||!key_for(file_id,key))return MEETING_RECORD_ERROR;
 esp_err_t err=nvs_get_blob(nvs,key,bytes,&n);if(err==ESP_ERR_NVS_NOT_FOUND)return MEETING_RECORD_MISSING;
 if(err!=ESP_OK||!meeting_record_decode(bytes,n,&decoded)||decoded.file_id!=file_id)return MEETING_RECORD_ERROR;
 *out=decoded;return MEETING_RECORD_OK;
}
static bool write_and_verify(nvs_handle_t nvs,const meeting_record *record)
{
 char key[10];uint8_t data[MEETING_RECORD_SIZE];meeting_record verify;
 if(!key_for(record->file_id,key)||!meeting_record_encode(record,data))return false;
 if(nvs_set_blob(nvs,key,data,sizeof(data))!=ESP_OK||nvs_commit(nvs)!=ESP_OK)return false;
 return meeting_record_nvs_read(nvs,record->file_id,&verify)==MEETING_RECORD_OK && verify.end_intent==record->end_intent && verify.recorded_at_present==record->recorded_at_present && verify.recorded_at==record->recorded_at && verify.time_trust==record->time_trust && !memcmp(verify.meeting_id,record->meeting_id,16);
}
bool meeting_record_nvs_create(nvs_handle_t nvs,const meeting_record *record)
{
 if(!record||record->end_intent!=0)return false;
 meeting_record existing;meeting_record_status status=meeting_record_nvs_read(nvs,record->file_id,&existing);
 if(status==MEETING_RECORD_OK){
  if(existing.end_intent!=0||existing.recorded_at_present!=record->recorded_at_present||existing.recorded_at!=record->recorded_at||existing.time_trust!=record->time_trust||memcmp(existing.meeting_id,record->meeting_id,16))return false;
  /* A failed commit may leave staged data visible. Recommit on retry. */
  return write_and_verify(nvs,&existing);
 }
 if(status!=MEETING_RECORD_MISSING)return false;
 return write_and_verify(nvs,record);
}
bool meeting_record_nvs_end_intent(nvs_handle_t nvs,uint32_t file_id,uint8_t intent)
{
 if(intent<1||intent>4)return false;
 meeting_record record;if(meeting_record_nvs_read(nvs,file_id,&record)!=MEETING_RECORD_OK)return false;
 if(record.end_intent)return record.end_intent==intent&&write_and_verify(nvs,&record);
 record.end_intent=intent;return write_and_verify(nvs,&record);
}
