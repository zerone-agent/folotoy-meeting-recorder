#include "meeting_credential_reset_nvs.h"
#include "meeting_credential_reset_config_nvs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CAP MEETING_CONFIG_RECORD_MAX
static uint8_t staged[3][CAP],durable[3][CAP];
static size_t staged_len[3],durable_len[3];
static int fail_erase_commit,erase_staged;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size)
{
 assert(h==1||h==2);assert(!strcmp(key,h==1?"active":"pending"));
 if(!staged_len[h])return ESP_ERR_NVS_NOT_FOUND;
 if(!out){*size=staged_len[h];return ESP_OK;}
 if(*size<staged_len[h])return ESP_ERR_NVS_INVALID_LENGTH;
 memcpy(out,staged[h],staged_len[h]);*size=staged_len[h];return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size)
{
 assert((h==1||h==2)&&!strcmp(key,h==1?"active":"pending")&&size<=CAP);
 memcpy(staged[h],data,size);staged_len[h]=size;return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t h,const char *key)
{
 assert(h==1&&!strcmp(key,"active"));if(!staged_len[h])return ESP_ERR_NVS_NOT_FOUND;
 staged_len[h]=0;erase_staged=1;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h)
{
 if(h==1&&erase_staged&&fail_erase_commit){fail_erase_commit=0;return 99;}
 memcpy(durable[h],staged[h],staged_len[h]);durable_len[h]=staged_len[h];
 if(h==1)erase_staged=0;return ESP_OK;
}
static bool ca(const char *pem,uint8_t hash[32],void *ctx)
{(void)ctx;if(strcmp(pem,"dummy"))return false;memset(hash,1,32);return true;}
static const uint8_t json[]="{\"version\":2,\"ssid\":\"wifi\",\"password\":\"12345678\",\"origin\":\"https://example.com\",\"deviceToken\":\"abcdefghijklmnopqrstuvwxyz012345\",\"deviceId\":\"device_1\",\"credentialId\":\"653bb1245e828fcda4fa53fcd5a3def5bd7654e651f54b4132b73d74e64435c4\",\"caPem\":\"dummy\",\"caSha256\":\"0101010101010101010101010101010101010101010101010101010101010101\",\"utc\":1800000000,\"timezoneMinutes\":480}";
int main(void)
{
 nvs_handle_t config_handle=1;meeting_config_store_io config_io;assert(meeting_config_store_nvs_bind(&config_handle,&config_io));
 uint8_t scratch[MEETING_CONFIG_RECORD_MAX];meeting_config config;
 assert(meeting_config_store_commit(&config_io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
 assert(meeting_config_parse(json,sizeof(json)-1,ca,NULL,&config));meeting_reset_record id;assert(meeting_reset_identity_from_config(&config,"event_1",&id));
 meeting_reset_config_nvs credentials={.config_handle=1,.validate_ca=ca};
 meeting_reset_nvs storage={.handle=2,.credential_ctx=&credentials,.credential_state=meeting_reset_config_state,.clear_credentials=meeting_reset_config_clear};meeting_reset_io io;assert(meeting_reset_nvs_bind(&storage,&io));
 fail_erase_commit=1;assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR);
 assert(!staged_len[1]&&durable_len[1]&&durable_len[2]);
 /* Power loss restores the old credential and the durable pending intent. */
 memcpy(staged[1],durable[1],durable_len[1]);staged_len[1]=durable_len[1];erase_staged=0;
 assert(meeting_reset_begin(&io,&id)==MEETING_RESET_OK&&!durable_len[1]);
 meeting_reset_record saved;assert(meeting_reset_decode(durable[2],&saved)&&saved.state==MEETING_RESET_LOCAL_CLEARED);
 assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_OK);
 assert(meeting_reset_decode(durable[2],&saved)&&saved.state==MEETING_RESET_REVOKED);
 /* Retry without a power cycle must commit the visible staged erase as well. */
 memset(staged_len,0,sizeof(staged_len));memset(durable_len,0,sizeof(durable_len));erase_staged=0;
 assert(meeting_config_store_commit(&config_io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
 fail_erase_commit=1;assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR);
 assert(!staged_len[1]&&durable_len[1]);
 assert(meeting_reset_begin(&io,&id)==MEETING_RESET_OK&&!durable_len[1]);
 return 0;
}
