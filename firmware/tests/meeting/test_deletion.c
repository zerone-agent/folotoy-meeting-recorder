#include "meeting_deletion.h"
#include <assert.h>
#include <string.h>
typedef struct {uint8_t data[2][MEETING_DELETION_SIZE];int present[2],fail_write,torn,fail_read;} fixture;
static int rd(void *ctx,unsigned slot,uint8_t out[MEETING_DELETION_SIZE]){
 fixture *f=ctx;if(f->fail_read)return -1;if(!f->present[slot])return 1;memcpy(out,f->data[slot],MEETING_DELETION_SIZE);return 0;
}
static bool wr(void *ctx,unsigned slot,const uint8_t data[MEETING_DELETION_SIZE]){
 fixture *f=ctx;if(f->fail_write==1)return false;memcpy(f->data[slot],data,MEETING_DELETION_SIZE);f->present[slot]=1;
 if(f->torn)f->data[slot][30]^=1;
 return f->fail_write!=2;
}
int main(void){
 fixture f={0};meeting_deletion_io io={&f,rd,wr};meeting_deletion a={.generation=1,.file_id=42,.meeting_id={1},.operation_id={2}},b;unsigned slot;
 assert(meeting_deletion_load(&io,&b,&slot)==MEETING_DELETION_EMPTY);
 meeting_deletion invalid=a;invalid.local_done=true;assert(!meeting_deletion_commit(&io,&invalid));
 invalid=a;invalid.remote=(meeting_delete_remote)-1;assert(!meeting_deletion_commit(&io,&invalid));
 f.fail_write=1;assert(!meeting_deletion_commit(&io,&a));assert(meeting_deletion_load(&io,&b,&slot)==MEETING_DELETION_EMPTY);
 f.fail_write=2;assert(!meeting_deletion_commit(&io,&a));f.fail_write=0;
 assert(meeting_deletion_load(&io,&b,&slot)==MEETING_DELETION_OK&&b.generation==1&&!b.local_done&&b.remote==MEETING_DELETE_REMOTE_PENDING);
 b.generation=2;b.local_done=true;assert(meeting_deletion_commit(&io,&b));
 assert(meeting_deletion_load(&io,&a,&slot)==MEETING_DELETION_OK&&a.local_done&&a.remote==MEETING_DELETE_REMOTE_PENDING);
 b=a;b.generation++;b.local_done=false;assert(!meeting_deletion_commit(&io,&b));
 b=a;b.generation++;b.operation_id[0]++;assert(!meeting_deletion_commit(&io,&b));
 b=a;b.generation++;b.remote=MEETING_DELETE_REMOTE_COMMITTED;assert(meeting_deletion_commit(&io,&b));
 assert(meeting_deletion_load(&io,&a,&slot)==MEETING_DELETION_OK&&a.local_done&&a.remote==MEETING_DELETE_REMOTE_COMMITTED);
 b=a;b.generation++;b.remote=MEETING_DELETE_REMOTE_CANCELLED;assert(!meeting_deletion_commit(&io,&b));
 b=a;b.generation++;f.torn=1;assert(!meeting_deletion_commit(&io,&b));f.torn=0;
 assert(meeting_deletion_load(&io,&b,&slot)==MEETING_DELETION_DAMAGED);assert(!meeting_deletion_commit(&io,&a));
 f.fail_read=1;assert(meeting_deletion_load(&io,&b,&slot)==MEETING_DELETION_IO_ERROR);
 fixture legacy={0};meeting_deletion_io legacy_io={&legacy,rd,wr};
 meeting_deletion local={.generation=1,.file_id=99,.meeting_id={1},.operation_id={2},.legacy_local_only=true};
 assert(meeting_deletion_commit(&legacy_io,&local));assert(!memcmp(legacy.data[0],"MDL2",4));
 assert(meeting_deletion_load(&legacy_io,&a,&slot)==MEETING_DELETION_OK&&a.legacy_local_only&&!a.local_done);
 a.generation++;a.local_done=true;assert(meeting_deletion_commit(&legacy_io,&a));
 b=a;b.generation++;b.legacy_local_only=false;assert(!meeting_deletion_commit(&legacy_io,&b));
 b=a;b.generation++;b.remote=MEETING_DELETE_REMOTE_CANCELLED;assert(!meeting_deletion_commit(&legacy_io,&b));
 fixture empty={0};meeting_deletion_io empty_io={&empty,rd,wr};
 meeting_deletion zero={.generation=1,.file_id=100,.meeting_id={3},.operation_id={4},.empty_local_only=true};
 assert(meeting_deletion_commit(&empty_io,&zero));assert(meeting_deletion_load(&empty_io,&a,&slot)==MEETING_DELETION_OK&&a.empty_local_only&&!a.legacy_local_only);
 b=a;b.generation++;b.empty_local_only=false;b.legacy_local_only=true;assert(!meeting_deletion_commit(&empty_io,&b));
 return 0;
}
