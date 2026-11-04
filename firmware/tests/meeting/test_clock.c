#include "meeting_clock.h"
#include <assert.h>

typedef struct { int status,write_ok,set_ok,writes,sets; int64_t floor; } fixture;
static int read_floor(void *ctx,int64_t *floor){fixture *f=ctx;*floor=f->floor;return f->status;}
static bool commit(void *ctx,int64_t floor){fixture *f=ctx;f->writes++;if(!f->write_ok)return false;f->floor=floor;f->status=0;return true;}
static bool set_time(void *ctx,int64_t epoch){fixture *f=ctx;(void)epoch;f->sets++;return f->set_ok;}
int main(void)
{
    fixture f={.status=1,.write_ok=1,.set_ok=1};meeting_clock_io io={&f,read_floor,commit,set_time};meeting_clock c;
    meeting_clock_reset(&c);assert(!meeting_clock_valid(&c,1800000000,1000));
    assert(!meeting_clock_bootstrap(&c,&io,false,true,true,1800000000,1000));assert(!f.writes&&!f.sets);
    assert(!meeting_clock_bootstrap(&c,&io,true,false,true,1800000000,1000));assert(!f.writes);
    assert(!meeting_clock_bootstrap(&c,&io,true,true,false,1800000000,1000));assert(!f.writes);
    f.status=-1;assert(!meeting_clock_bootstrap(&c,&io,true,true,true,1800000000,1000));assert(!f.writes);
    f.status=1;f.write_ok=0;assert(!meeting_clock_bootstrap(&c,&io,true,true,true,1800000000,1000));assert(!f.sets&&!c.trusted);
    f.write_ok=1;f.set_ok=0;assert(!meeting_clock_bootstrap(&c,&io,true,true,true,1800000000,1000));assert(!c.trusted);
    f.set_ok=1;assert(!meeting_clock_bootstrap(&c,&io,true,true,false,1799999999,1000));
    assert(meeting_clock_bootstrap(&c,&io,true,true,false,1800000000,1000));
    assert(meeting_clock_valid(&c,1800000002,3000));
    assert(meeting_clock_checkpoint(&c,&io,1800000002,3000)&&f.floor==1800000002);
    assert(!meeting_clock_valid(&c,1799999999,4000)&&!c.trusted);
    assert(!meeting_clock_bootstrap(&c,&io,true,true,false,1800000001,5000));
    assert(meeting_clock_bootstrap(&c,&io,true,true,false,1800000002,5000));
    assert(!meeting_clock_valid(&c,1800000003,4999)&&!c.trusted); /* reboot or monotonic reset */
    assert(meeting_clock_bootstrap(&c,&io,true,true,false,1800000002,5000));
    f.write_ok=0;assert(!meeting_clock_checkpoint(&c,&io,1800000003,6000)&&!c.trusted);
    f.write_ok=1;
    assert(meeting_clock_bootstrap(&c,&io,true,true,false,1800000003,6000));
    assert(!meeting_clock_valid(&c,1800000010,7000)&&!c.trusted); /* excessive forward drift */
    assert(!meeting_clock_bootstrap(&c,&io,true,true,true,4102444801LL,8000));
    return 0;
}
