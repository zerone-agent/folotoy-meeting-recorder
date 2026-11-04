#include "meeting_record.h"
#include <string.h>
static uint32_t checksum(const uint8_t *p,unsigned n){uint32_t c=~0u;for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static uint32_t get32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static bool valid(const meeting_record *v){
 if(!v||!v->file_id||v->end_intent>4)return false;
 unsigned any=0;for(unsigned i=0;i<16;i++)any|=v->meeting_id[i];
 if(!any)return false;
 if(!v->recorded_at_present)return v->time_trust==0&&v->recorded_at==0;
 return v->time_trust==2&&v->recorded_at>=1735689600LL&&v->recorded_at<=4102444800LL;
}
bool meeting_record_encode(const meeting_record *v,uint8_t out[MEETING_RECORD_SIZE]){
 if(!out||!valid(v))return false;
 memset(out,0,MEETING_RECORD_SIZE);memcpy(out,"MR02",4);put32(out+4,v->file_id);memcpy(out+8,v->meeting_id,16);out[24]=v->end_intent;out[25]=v->time_trust;
 uint64_t epoch=(uint64_t)v->recorded_at;for(unsigned i=0;i<8;i++)out[28+i]=(uint8_t)(epoch>>(8*i));put32(out+44,checksum(out,44));return true;
}
bool meeting_record_decode(const uint8_t *in,size_t len,meeting_record *v){
 if(!in||!v)return false;
 meeting_record tmp={0};
 if(len==MEETING_RECORD_LEGACY_SIZE&&!memcmp(in,"MR01",4)){
  if(in[25]||in[26]||in[27]||get32(in+28)!=checksum(in,28))return false;
 }else if(len==MEETING_RECORD_SIZE&&!memcmp(in,"MR02",4)){
  if(in[26]||in[27]||get32(in+44)!=checksum(in,44))return false;
  for(unsigned i=36;i<44;i++)if(in[i])return false;
  uint64_t epoch=0;for(unsigned i=0;i<8;i++)epoch|=(uint64_t)in[28+i]<<(8*i);
  if(epoch>4102444800ULL)return false;
  tmp.recorded_at=(int64_t)epoch;tmp.recorded_at_present=epoch!=0;tmp.time_trust=in[25];
 }else return false;
 tmp.file_id=get32(in+4);memcpy(tmp.meeting_id,in+8,16);tmp.end_intent=in[24];if(!valid(&tmp))return false;*v=tmp;return true;
}
