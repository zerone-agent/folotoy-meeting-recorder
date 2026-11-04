#include "meeting_time_anchor.h"
#include <string.h>
static uint32_t crc32(const uint8_t *p,unsigned n){uint32_t c=~0u;for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static uint32_t get32(const uint8_t *p){uint32_t x=0;for(unsigned i=0;i<4;i++)x|=(uint32_t)p[i]<<(8*i);return x;}
static bool structural(const meeting_time_anchor *a){if(!a||a->key.public_key[0]!=4)return false;uint8_t n=0;for(unsigned i=0;i<32;i++)n|=a->origin_sha256[i];return n!=0;}
bool meeting_time_anchor_encode(const meeting_time_anchor *a,uint8_t out[MEETING_TIME_ANCHOR_SIZE]){
 if(!out||!structural(a))return false;
 memset(out,0,MEETING_TIME_ANCHOR_SIZE);memcpy(out,"MTA1",4);memcpy(out+4,a->origin_sha256,32);memcpy(out+36,a->key.public_key,65);
 uint32_t c=crc32(out,104);for(unsigned i=0;i<4;i++)out[104+i]=(uint8_t)(c>>(8*i));return true;
}
bool meeting_time_anchor_decode(const uint8_t data[MEETING_TIME_ANCHOR_SIZE],meeting_time_anchor *out){
 if(!data||!out||memcmp(data,"MTA1",4)||data[101]||data[102]||data[103]||get32(data+104)!=crc32(data,104))return false;
 meeting_time_anchor a;memcpy(a.origin_sha256,data+4,32);memcpy(a.key.public_key,data+36,65);if(!structural(&a))return false;*out=a;return true;
}
meeting_time_anchor_result meeting_time_anchor_install(meeting_time_anchor_session *s,const meeting_time_anchor_io *io,
 const meeting_time_anchor *a,meeting_time_anchor_authorized authorized,void *auth_ctx,meeting_time_key_check check,void *key_ctx){
 if(!s||!io||!io->read||!io->write_sync||!a||!authorized||!check)return MEETING_ANCHOR_ERROR;
 uint8_t expected[MEETING_TIME_ANCHOR_SIZE],existing[MEETING_TIME_ANCHOR_SIZE];meeting_time_anchor old;
 if(!meeting_time_anchor_encode(a,expected))return MEETING_ANCHOR_ERROR;
 if(!authorized(auth_ctx,a))return MEETING_ANCHOR_DENIED;
 if(!check(key_ctx,&a->key))return MEETING_ANCHOR_ERROR;
 int status=io->read(io->ctx,existing);
 if(status==0){if(!meeting_time_anchor_decode(existing,&old)||!check(key_ctx,&old.key))return MEETING_ANCHOR_ERROR;if(memcmp(existing,expected,sizeof expected))return MEETING_ANCHOR_CONFLICT;}
 else if(status!=1)return MEETING_ANCHOR_ERROR;
 if(!authorized(auth_ctx,a))return MEETING_ANCHOR_DENIED;
 s->uncertain=true;
 if(!io->write_sync(io->ctx,expected))return MEETING_ANCHOR_ERROR;
 if(!authorized(auth_ctx,a))return MEETING_ANCHOR_DENIED;
 if(io->read(io->ctx,existing)!=0||memcmp(existing,expected,sizeof expected))return MEETING_ANCHOR_ERROR;
 if(!authorized(auth_ctx,a))return MEETING_ANCHOR_DENIED;
 s->uncertain=false;return MEETING_ANCHOR_OK;
}
meeting_time_anchor_result meeting_time_anchor_load(const meeting_time_anchor_session *s,const meeting_time_anchor_io *io,
 const uint8_t origin[32],meeting_time_key_check check,void *key_ctx,meeting_time_anchor *out){
 if(!s||s->uncertain||!io||!io->read||!origin||!check||!out)return MEETING_ANCHOR_ERROR;
 uint8_t raw[MEETING_TIME_ANCHOR_SIZE];meeting_time_anchor value;int status=io->read(io->ctx,raw);
 if(status==1)return MEETING_ANCHOR_ABSENT;
 if(status!=0||!meeting_time_anchor_decode(raw,&value)||!check(key_ctx,&value.key))return MEETING_ANCHOR_ERROR;
 if(memcmp(value.origin_sha256,origin,32))return MEETING_ANCHOR_CONFLICT;
 *out=value;return MEETING_ANCHOR_OK;
}
