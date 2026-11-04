#include "meeting_snapshot.h"
#include <assert.h>
#include <string.h>
typedef struct {uint8_t slots[2][MEETING_SNAPSHOT_SIZE];int exists[2];int failure;} fixture;
static int rd(void *p,unsigned i,uint8_t out[MEETING_SNAPSHOT_SIZE]){fixture *f=p;if(f->failure==1)return -1;if(!f->exists[i])return 1;memcpy(out,f->slots[i],MEETING_SNAPSHOT_SIZE);return 0;}
static bool wr(void *p,unsigned i,const uint8_t *data){fixture *f=p;f->exists[i]=1;if(f->failure==2){memcpy(f->slots[i],data,12);return false;}memcpy(f->slots[i],data,MEETING_SNAPSHOT_SIZE);return true;}
int main(void){fixture f={0};meeting_snapshot_io io={&f,rd,wr};meeting_snapshot a={.generation=1,.valid_end=104,.frames=2,.reason=3},out;unsigned slot;memset(a.meeting_id,1,16);memset(a.source_sha256,2,32);assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_EMPTY);assert(meeting_snapshot_commit(&io,&a));a.generation=2;assert(meeting_snapshot_commit(&io,&a));assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_OK&&out.generation==2);a.generation=3;f.failure=2;assert(!meeting_snapshot_commit(&io,&a));f.failure=0;assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_DEGRADED);assert(!meeting_snapshot_commit(&io,&a));f.failure=1;assert(meeting_snapshot_load(&io,&out,&slot)==MEETING_SNAPSHOT_ERROR);return 0;}
