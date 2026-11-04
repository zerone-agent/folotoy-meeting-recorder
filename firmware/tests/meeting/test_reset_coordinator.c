#include "meeting_reset_coordinator.h"
#include "meeting_arbitration.h"
#include <assert.h>
#include <string.h>
typedef struct {
    bool session, admin, stopped, present, active, lose_on_write;
    unsigned stops, clears;
    uint8_t data[MEETING_RESET_RECORD_SIZE];
    meeting_arbitration arbitration;
} fixture;
static bool session(void *p){return ((fixture *)p)->session;}
static bool admin(void *p,const meeting_reset_record *id){(void)id;return ((fixture *)p)->admin;}
static bool stop(void *p){((fixture *)p)->stops++;return true;}
static bool stopped(void *p){return ((fixture *)p)->stopped;}
static bool acquire(void *p){return meeting_arbitration_maintenance_acquire(&((fixture *)p)->arbitration);}
static bool release(void *p){return meeting_arbitration_maintenance_release(&((fixture *)p)->arbitration);}
static int read_data(void *p,uint8_t *out){fixture *f=p;if(!f->present)return 1;memcpy(out,f->data,sizeof(f->data));return 0;}
static bool write_data(void *p,const uint8_t *data){fixture *f=p;memcpy(f->data,data,sizeof(f->data));f->present=true;if(f->lose_on_write)f->admin=false;return true;}
static int credential_state(void *p,const meeting_reset_record *id){(void)id;return ((fixture *)p)->active?0:1;}
static bool clear(void *p){fixture *f=p;f->clears++;f->active=false;return true;}
static meeting_reset_coordinator_io make_io(fixture *f){return (meeting_reset_coordinator_io){f,session,admin,stop,stopped,acquire,release,{f,read_data,write_data,credential_state,clear}};}
int main(void){
    fixture f={.session=true,.admin=false,.active=true};meeting_arbitration_init(&f.arbitration);
    meeting_reset_coordinator state={0};meeting_reset_coordinator_io io=make_io(&f);
    meeting_reset_record id={.origin="https://example.com",.device_id="device_1",.event_id="event_1",.state=MEETING_RESET_PENDING};memset(id.credential_id,'a',64);
    assert(meeting_reset_coordinator_start(&state,&io,&id)==MEETING_RESET_COORD_AUTH_LOST && f.stops==0);
    f.admin=true;assert(meeting_reset_coordinator_start(&state,&io,&id)==MEETING_RESET_COORD_WAIT_NETWORK);
    assert(meeting_reset_coordinator_poll(&state,&io)==MEETING_RESET_COORD_WAIT_NETWORK && f.clears==0);
    f.stopped=true;assert(meeting_arbitration_record_request(&f.arbitration));assert(meeting_arbitration_record_acquire(&f.arbitration));
    assert(meeting_reset_coordinator_poll(&state,&io)==MEETING_RESET_COORD_WAIT_RECORDING && f.clears==0);
    assert(meeting_arbitration_record_end(&f.arbitration));f.lose_on_write=true;
    assert(meeting_reset_coordinator_poll(&state,&io)==MEETING_RESET_COORD_AUTH_LOST && f.present && f.active && f.clears==0);
    f.admin=true;f.lose_on_write=false;
    assert(meeting_reset_coordinator_start(&state,&io,&id)==MEETING_RESET_COORD_WAIT_NETWORK);
    assert(meeting_reset_coordinator_poll(&state,&io)==MEETING_RESET_COORD_DONE && !f.active && f.clears==1);
    meeting_reset_record record;assert(meeting_reset_decode(f.data,&record) && record.state==MEETING_RESET_LOCAL_CLEARED);
    return 0;
}
