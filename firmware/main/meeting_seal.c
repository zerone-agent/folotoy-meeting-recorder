#include "meeting_seal.h"
#include <string.h>
static uint32_t crc32(const uint8_t *p,unsigned n){uint32_t c=~0u;for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static bool valid(const meeting_seal *s){if(!s||!s->file_id||!s->frames||s->frames>90000||s->bytes<98||s->reason>3)return false;unsigned any=0;for(unsigned i=0;i<16;i++)any|=s->meeting_id[i];return any!=0;}
bool meeting_seal_encode(const meeting_seal *s,uint8_t out[MEETING_SEAL_SIZE]){
 if(!out||!valid(s))return false;
 memset(out,0,MEETING_SEAL_SIZE);memcpy(out,"MS01",4);
 for(unsigned i=0;i<4;i++){out[4+i]=(uint8_t)(s->file_id>>(8*i));out[24+i]=(uint8_t)(s->frames>>(8*i));}
 memcpy(out+8,s->meeting_id,16);for(unsigned i=0;i<8;i++)out[28+i]=(uint8_t)(s->bytes>>(8*i));out[36]=s->reason;
 uint32_t crc=crc32(out,44);for(unsigned i=0;i<4;i++)out[44+i]=(uint8_t)(crc>>(8*i));return true;
}
bool meeting_seal_decode(const uint8_t in[MEETING_SEAL_SIZE],meeting_seal *out){
 if(!in||!out||memcmp(in,"MS01",4))return false;
 for(unsigned i=37;i<44;i++)if(in[i])return false;
 uint32_t crc=0;for(unsigned i=0;i<4;i++)crc|=(uint32_t)in[44+i]<<(8*i);if(crc!=crc32(in,44))return false;
 meeting_seal s={0};for(unsigned i=0;i<4;i++){s.file_id|=(uint32_t)in[4+i]<<(8*i);s.frames|=(uint32_t)in[24+i]<<(8*i);}
 memcpy(s.meeting_id,in+8,16);for(unsigned i=0;i<8;i++)s.bytes|=(uint64_t)in[28+i]<<(8*i);s.reason=in[36];
 if(!valid(&s))return false;
 *out=s;return true;
}
