#include "meeting_seal.h"
#include <assert.h>
#include <string.h>
int main(void){
 meeting_seal s={.file_id=42,.meeting_id={1},.frames=90000,.bytes=3266028,.reason=1},out;
 uint8_t bytes[MEETING_SEAL_SIZE];assert(meeting_seal_encode(&s,bytes)&&meeting_seal_decode(bytes,&out));
 assert(out.file_id==s.file_id&&out.frames==s.frames&&out.bytes==s.bytes&&out.reason==s.reason&&!memcmp(out.meeting_id,s.meeting_id,16));
 bytes[30]^=1;assert(!meeting_seal_decode(bytes,&out));bytes[30]^=1;
 s.frames=90001;assert(!meeting_seal_encode(&s,bytes));s.frames=0;assert(!meeting_seal_encode(&s,bytes));
 s.frames=1;s.bytes=0;assert(!meeting_seal_encode(&s,bytes));s.bytes=100;s.reason=4;assert(!meeting_seal_encode(&s,bytes));
 return 0;
}
