#include "meeting_cancel_request.h"
#include <assert.h>
#include <string.h>
typedef struct {bool cancelled,fail;int status,calls;const char *body;} fixture;
static bool stopped(void *ctx){return ((fixture *)ctx)->cancelled;}
static bool request(void *ctx,const char *method,const char *path,const char *type,const char *hash,const uint8_t *body,size_t body_len,int *status,char *out,size_t cap,size_t *len){
 fixture *f=ctx;f->calls++;assert(!strcmp(method,"POST")&&!strcmp(path,"/meeting-service/meetings/12345678-1234-4123-8123-123456789abc/cancel"));
 assert(type&&!strcmp(type,"application/json")&&!hash&&!body&&!body_len);if(f->fail)return false;
 *status=f->status;*len=strlen(f->body);assert(*len<=cap);memcpy(out,f->body,*len);return true;
}
int main(void){
 const uint8_t id[16]={0x12,0x34,0x56,0x78,0x12,0x34,0x41,0x23,0x81,0x23,0x12,0x34,0x56,0x78,0x9a,0xbc};
 fixture f={.status=200,.body="{\"state\":\"cancelled\"}"};meeting_upload_transport t={&f,stopped,request};
 assert(meeting_cancel_request(id,NULL,&t)==1&&f.calls==1);
 f.fail=true;assert(meeting_cancel_request(id,NULL,&t)==0);f.fail=false;
 f.status=409;assert(meeting_cancel_request(id,NULL,&t)==0);f.status=200;
 f.body="{\"state\":\"committed\"}";assert(meeting_cancel_request(id,NULL,&t)==0);
 meeting_manifest mismatch={0};assert(meeting_cancel_request(id,&mismatch,&t)==0);
 meeting_manifest expected={.count=1,.bytes=100,.reason=3};memcpy(expected.id,"12345678-1234-4123-8123-123456789abc",37);memset(expected.digest,7,32);
 f.body="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"state\":\"committed\",\"committed\":true,\"manifest\":{\"count\":1,\"bytes\":100,\"digest\":\"0707070707070707070707070707070707070707070707070707070707070707\",\"reason\":\"interrupted\",\"recordedAt\":null,\"timeTrust\":\"uncalibrated\"}}";
 assert(meeting_cancel_request(id,&expected,&t)==2);expected.digest[0]^=1;assert(meeting_cancel_request(id,&expected,&t)==0);
 f.cancelled=true;int calls=f.calls;assert(meeting_cancel_request(id,NULL,&t)==0&&f.calls==calls);
 return 0;
}
