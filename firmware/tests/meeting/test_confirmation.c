#include "meeting_confirmation.h"
#include <assert.h>
#include <string.h>

typedef struct { uint8_t data[MEETING_CONFIRMATION_SIZE]; int present, read_error, write_mode; } fixture;
static int read_slot(void *p,uint8_t out[MEETING_CONFIRMATION_SIZE]) {
    fixture *f=p;if(f->read_error)return -1;if(!f->present)return 1;memcpy(out,f->data,sizeof(f->data));return 0;
}
static bool write_slot(void *p,const uint8_t data[MEETING_CONFIRMATION_SIZE]) {
    fixture *f=p;if(f->write_mode==1)return false;memcpy(f->data,data,sizeof(f->data));f->present=1;
    if(f->write_mode==2){f->data[20]^=1;return true;}
    return f->write_mode!=3; /* commit may have happened despite an error */
}
int main(void) {
    meeting_confirmation expected={.file_id=42,.source_sha256={1},.manifest={.count=2,.bytes=4100,.digest={2},.reason=3}};
    memcpy(expected.manifest.id,"12345678-1234-4123-8123-123456789abc",37);
    meeting_snapshot snapshot={.valid_end=4100,.reason=3,.source_sha256={1},
        .meeting_id={0x12,0x34,0x56,0x78,0x12,0x34,0x41,0x23,0x81,0x23,0x12,0x34,0x56,0x78,0x9a,0xbc}};
    assert(meeting_confirmation_matches_snapshot(&expected,&snapshot));
    snapshot.reason=0;assert(!meeting_confirmation_matches_snapshot(&expected,&snapshot));snapshot.reason=3;
    snapshot.meeting_id[0]^=1;assert(!meeting_confirmation_matches_snapshot(&expected,&snapshot));snapshot.meeting_id[0]^=1;
    fixture f={0};meeting_confirmation_io io={&f,read_slot,write_slot};meeting_confirmation out;
    assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_EMPTY);
    f.write_mode=1;assert(!meeting_confirmation_commit(&io,&expected));assert(!f.present);
    f.write_mode=2;assert(!meeting_confirmation_commit(&io,&expected));
    assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);
    assert(!meeting_confirmation_commit(&io,&expected)); /* corruption is never overwritten */
    f.present=0;f.write_mode=3;assert(!meeting_confirmation_commit(&io,&expected));
    assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_OK);
    assert(meeting_receipt_matches(&out.manifest,&expected.manifest,true));
    assert(out.file_id==42&&!memcmp(out.source_sha256,expected.source_sha256,32));
    f.write_mode=0;assert(meeting_confirmation_commit(&io,&expected)); /* re-commit after ambiguous write */
    meeting_confirmation different=expected;different.source_sha256[0]=3;
    assert(!meeting_confirmation_commit(&io,&different));
    f.read_error=1;assert(meeting_confirmation_load(&io,&out)==MEETING_CONFIRMATION_ERROR);
    assert(!meeting_confirmation_commit(&io,&expected));
    return 0;
}
