#include "meeting_credential_reset.h"
#include <assert.h>
#include <string.h>

typedef struct {
    uint8_t data[MEETING_RESET_RECORD_SIZE];
    int present,active,reads,writes,clears,fail_read,fail_write,fail_after_write,fail_clear,lie_clear;
} fixture;
static int read_record(void *ctx,uint8_t *out)
{fixture *f=ctx;f->reads++;if(f->fail_read==f->reads)return -1;if(!f->present)return 1;memcpy(out,f->data,sizeof(f->data));return 0;}
static bool write_record(void *ctx,const uint8_t *data)
{fixture *f=ctx;f->writes++;if(f->fail_write==f->writes)return false;memcpy(f->data,data,sizeof(f->data));f->present=1;return f->fail_after_write!=f->writes;}
static int credential_state(void *ctx,const meeting_reset_record *record)
{fixture *f=ctx;(void)record;return f->active?0:1;}
static bool clear_credentials(void *ctx)
{fixture *f=ctx;f->clears++;if(f->fail_clear)return false;if(!f->lie_clear)f->active=0;return true;}
static meeting_reset_io io_for(fixture *f)
{return (meeting_reset_io){f,read_record,write_record,credential_state,clear_credentials};}
static meeting_reset_record identity(void)
{
    meeting_reset_record v={.origin="https://example.com",.device_id="device_1",.event_id="event_1",.state=MEETING_RESET_PENDING};
    memset(v.credential_id,'a',64);return v;
}
int main(void)
{
    meeting_reset_record id=identity(),decoded;uint8_t raw[MEETING_RESET_RECORD_SIZE];
    assert(meeting_reset_encode(&id,raw)&&meeting_reset_decode(raw,&decoded));
    raw[100]^=1;assert(!meeting_reset_decode(raw,&decoded));
    meeting_reset_record bad=id;strcpy(bad.event_id,"../bad");assert(!meeting_reset_encode(&bad,raw));
    /* The write may persist even if its acknowledgment is lost. No erase is
     * allowed in that call; a retry reconciles the durable pending record. */
    fixture ambiguous={.active=1,.fail_after_write=1};meeting_reset_io ambiguous_io=io_for(&ambiguous);
    assert(meeting_reset_begin(&ambiguous_io,&id)==MEETING_RESET_ERROR&&ambiguous.present&&ambiguous.active&&ambiguous.clears==0);
    ambiguous.fail_after_write=0;ambiguous.fail_write=ambiguous.writes+1;
    assert(meeting_reset_begin(&ambiguous_io,&id)==MEETING_RESET_ERROR&&ambiguous.active&&ambiguous.clears==0);
    ambiguous.fail_write=0;assert(meeting_reset_begin(&ambiguous_io,&id)==MEETING_RESET_OK&&!ambiguous.active);
    fixture f={.active=1,.fail_write=1};meeting_reset_io io=io_for(&f);
    assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR&&f.active&&f.clears==0);
    f.fail_write=0;f.fail_clear=1;
    assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR&&f.active&&f.present&&f.clears==1);
    assert(meeting_reset_decode(f.data,&decoded)&&decoded.state==MEETING_RESET_PENDING);
    meeting_reset_record other=id;strcpy(other.event_id,"other");
    assert(meeting_reset_begin(&io,&other)==MEETING_RESET_CONFLICT&&f.active);
    assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_ERROR&&f.active);
    f.fail_clear=0;f.lie_clear=1;
    assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR&&f.active);
    f.lie_clear=0;f.fail_write=f.writes+2;
    assert(meeting_reset_begin(&io,&id)==MEETING_RESET_ERROR&&!f.active);
    assert(meeting_reset_decode(f.data,&decoded)&&decoded.state==MEETING_RESET_PENDING);
    int clears=f.clears;f.fail_write=0;
    assert(meeting_reset_begin(&io,&id)==MEETING_RESET_OK&&!f.active&&f.clears==clears+1);
    assert(meeting_reset_decode(f.data,&decoded)&&decoded.state==MEETING_RESET_LOCAL_CLEARED);
    assert(meeting_reset_ack(&io,&id,false)==MEETING_RESET_ERROR);
    assert(meeting_reset_ack(&io,&other,true)==MEETING_RESET_CONFLICT);
    f.fail_write=f.writes+2;assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_ERROR);
    assert(meeting_reset_decode(f.data,&decoded)&&decoded.state==MEETING_RESET_LOCAL_CLEARED);
    f.fail_write=0;assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_OK);
    assert(meeting_reset_ack(&io,&id,true)==MEETING_RESET_OK);
    assert(meeting_reset_decode(f.data,&decoded)&&decoded.state==MEETING_RESET_REVOKED);
    return 0;
}
