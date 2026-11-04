#include "meeting_credential_reset_nvs.h"
#include <assert.h>
#include <string.h>
static uint8_t blob[MEETING_RESET_RECORD_SIZE];static size_t len;static int fail_commit,active=1;
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size)
{(void)h;assert(!strcmp(key,"pending"));if(!len)return ESP_ERR_NVS_NOT_FOUND;if(*size<len)return ESP_ERR_NVS_INVALID_LENGTH;memcpy(out,blob,len);*size=len;return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size)
{(void)h;assert(!strcmp(key,"pending")&&size==sizeof(blob));memcpy(blob,data,size);len=size;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;return fail_commit?99:ESP_OK;}
static int state(void *ctx,const meeting_reset_record *v){(void)ctx;(void)v;return active?0:1;}
static bool clear(void *ctx){(void)ctx;active=0;return true;}
int main(void)
{
 meeting_reset_nvs nvs={.handle=1,.credential_state=state,.clear_credentials=clear};meeting_reset_io io;
 assert(meeting_reset_nvs_bind(&nvs,&io));meeting_reset_record id={.origin="https://example.com",.device_id="dev",.event_id="event",.state=MEETING_RESET_PENDING};memset(id.credential_id,'a',64);
 fail_commit=1;assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR&&active);
 fail_commit=0;assert(meeting_reset_begin(&io,&id)==MEETING_RESET_OK&&!active);
 meeting_reset_record value;assert(meeting_reset_decode(blob,&value)&&value.state==MEETING_RESET_LOCAL_CLEARED);
 assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_OK);
 assert(meeting_reset_decode(blob,&value)&&value.state==MEETING_RESET_REVOKED);
 len=1;assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR);
 return 0;
}
