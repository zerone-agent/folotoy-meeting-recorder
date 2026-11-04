#include "meeting_cleanup.h"
#include <assert.h>
#include <string.h>

typedef struct {uint8_t bytes[MEETING_CONFIRMATION_SIZE];bool present,fail,corrupt;} slot;
typedef struct {bool audio,index,verified,closed;int fail_at,steps,cancel_at,checks;} fs;
static int read_slot(void *ctx,uint8_t out[MEETING_CONFIRMATION_SIZE]){slot *s=ctx;if(!s->present)return 1;memcpy(out,s->bytes,sizeof(s->bytes));return 0;}
static bool write_slot(void *ctx,const uint8_t data[MEETING_CONFIRMATION_SIZE]){slot *s=ctx;if(s->fail)return false;memcpy(s->bytes,data,sizeof(s->bytes));s->present=true;if(s->corrupt)s->bytes[20]^=1;return true;}
static bool step(fs *f){f->steps++;return f->fail_at!=f->steps;}
static bool cancelled(void *ctx){fs *f=ctx;f->checks++;return f->cancel_at&&f->checks>=f->cancel_at;}
static int exists(void *ctx){return ((fs *)ctx)->audio?0:1;}
static bool verify(void *ctx,const meeting_confirmation *r,const meeting_snapshot *s){fs *f=ctx;(void)r;(void)s;return step(f)&&f->verified;}
static bool close_readers(void *ctx){fs *f=ctx;f->closed=step(f);return f->closed;}
static bool remove_audio(void *ctx){fs *f=ctx;if(!step(f))return false;assert(f->closed);f->audio=false;return true;}
static bool remove_index(void *ctx){fs *f=ctx;if(!step(f))return false;f->index=false;return true;}
static bool check(void *ctx){fs *f=ctx;return step(f)&&!f->audio&&!f->index;}
int main(void)
{
    meeting_confirmation receipt={.file_id=7,.source_sha256={1},.manifest={.count=2,.bytes=4100,.digest={2},.reason=3}};
    memcpy(receipt.manifest.id,"12345678-1234-4123-8123-123456789abc",37);
    meeting_snapshot snapshot={.valid_end=4100,.reason=3,.source_sha256={1},
        .meeting_id={0x12,0x34,0x56,0x78,0x12,0x34,0x41,0x23,0x81,0x23,0x12,0x34,0x56,0x78,0x9a,0xbc}};
    slot ack={0},intent={0};meeting_confirmation_io a={&ack,read_slot,write_slot},i={&intent,read_slot,write_slot};
    assert(meeting_confirmation_commit(&a,&receipt));
    for(int failure=1;failure<=5;failure++){
        intent=(slot){0};fs f={.audio=true,.index=true,.verified=true,.fail_at=failure};
        meeting_cleanup_io io={&f,cancelled,exists,verify,close_readers,remove_audio,remove_index,check};
        assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_ERROR);
        if(!f.audio)assert(intent.present); /* never remove before durable intent */
        f.fail_at=0;f.steps=0;f.closed=false;
        assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_DONE);
        assert(!f.audio&&!f.index&&intent.present);
        f.steps=0;f.closed=false;assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_DONE);
    }
    fs f={.audio=true,.index=true,.verified=true};meeting_cleanup_io io={&f,cancelled,exists,verify,close_readers,remove_audio,remove_index,check};
    intent=(slot){.fail=true};assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_ERROR&&f.audio&&f.index);
    intent=(slot){0};f.audio=false;assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_ERROR&&f.index);
    f.audio=true;f.verified=false;assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_ERROR&&f.audio);
    f=(fs){.audio=true,.index=true,.verified=false,.cancel_at=2};
    assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_PAUSED&&f.audio&&!intent.present);
    intent=(slot){0};f=(fs){.audio=true,.index=true,.verified=true,.cancel_at=5};
    assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_PAUSED);
    assert(!f.audio&&f.index&&intent.present);
    f.cancel_at=0;f.closed=false;assert(meeting_cleanup_run(7,&snapshot,&a,&i,&io)==MEETING_CLEANUP_DONE);
    return 0;
}
