#include "meeting_record.h"
#include <assert.h>
#include <string.h>
static uint32_t crc(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
int main(void){
 meeting_record a={.file_id=77,.end_intent=2},b;memset(a.meeting_id,5,16);
 uint8_t x[MEETING_RECORD_SIZE],bad[MEETING_RECORD_SIZE];
 assert(meeting_record_encode(&a,x)&&meeting_record_decode(x,sizeof x,&b));
 assert(b.file_id==a.file_id&&b.end_intent==2&&!b.recorded_at_present&&!memcmp(b.meeting_id,a.meeting_id,16));
 for(unsigned i=0;i<sizeof x;i++){memcpy(bad,x,sizeof x);bad[i]^=1;assert(!meeting_record_decode(bad,sizeof bad,&b));}
 a.recorded_at_present=true;a.recorded_at=1800000000;a.time_trust=2;
 assert(meeting_record_encode(&a,x)&&meeting_record_decode(x,sizeof x,&b)&&b.recorded_at_present&&b.recorded_at==a.recorded_at&&b.time_trust==2);
 a.time_trust=0;assert(!meeting_record_encode(&a,x));a.time_trust=2;a.recorded_at=42;assert(!meeting_record_encode(&a,x));
 a.recorded_at_present=false;a.recorded_at=0;a.time_trust=0;a.end_intent=4;assert(meeting_record_encode(&a,x)&&meeting_record_decode(x,sizeof x,&b)&&b.end_intent==4);
 a.end_intent=5;assert(!meeting_record_encode(&a,x));a.file_id=0;assert(!meeting_record_encode(&a,x));
 uint8_t legacy[MEETING_RECORD_LEGACY_SIZE]={0};memcpy(legacy,"MR01",4);legacy[4]=77;memset(legacy+8,5,16);legacy[24]=2;
 uint32_t c=crc(legacy,28);for(unsigned i=0;i<4;i++)legacy[28+i]=(uint8_t)(c>>(8*i));
 assert(meeting_record_decode(legacy,sizeof legacy,&b)&&b.file_id==77&&!b.recorded_at_present&&b.time_trust==0);
 legacy[8]^=1;assert(!meeting_record_decode(legacy,sizeof legacy,&b));
 return 0;
}
