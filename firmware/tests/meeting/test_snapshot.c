#include "meeting_snapshot.h"
#include <assert.h>
#include <string.h>
int main(void){meeting_snapshot a={.generation=1,.valid_end=104,.frames=2,.reason=3},b;memset(a.meeting_id,1,16);memset(a.source_sha256,2,32);uint8_t data[MEETING_SNAPSHOT_SIZE],bad[MEETING_SNAPSHOT_SIZE];assert(meeting_snapshot_encode(&a,data));assert(meeting_snapshot_decode(data,&b));assert(b.generation==1&&b.valid_end==104&&b.frames==2&&b.reason==3&&!memcmp(a.source_sha256,b.source_sha256,32));for(unsigned i=0;i<sizeof data;i++){memcpy(bad,data,sizeof data);bad[i]^=1;assert(!meeting_snapshot_decode(bad,&b));}a.frames=0;assert(!meeting_snapshot_encode(&a,data));return 0;}
