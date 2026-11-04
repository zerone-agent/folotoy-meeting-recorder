#include "meeting_worker.h"
#include "meeting_runtime.h"
#include <assert.h>

static int online=1,clock_ok=1,cancelled_flag,calls,drop_after,empty_first;
static int simulate_removal,missing_metadata;static bool removed[4];
static uint32_t seen[4];static meeting_upload_result answers[4];
static bool has_ip(void *ctx){(void)ctx;return online;}
static bool time_ok(void *ctx){(void)ctx;return clock_ok;}
static bool cancelled(void *ctx){(void)ctx;return cancelled_flag;}
size_t sc_store_count(void){size_t count=0;for(int id=1;id<=3;id++)if(!removed[id])count++;return count;}
size_t sc_store_list_range(size_t start,sc_file_entry_t *out,size_t capacity)
{
    sc_file_entry_t items[3]={{.file_id=3,.size=100},{.file_id=2,.size=100},{.file_id=1,.size=empty_first?0:100}};
    size_t n=0,at=0;for(size_t i=0;i<3&&n<capacity;i++){
        if(removed[items[i].file_id])continue;
        if(at++>=start)out[n++]=items[i];
    }return n;
}
bool sc_store_record_time(uint32_t id,bool *present,int64_t *epoch,uint8_t *trust){if(missing_metadata&&id==2)return false;*present=id==2;*epoch=id==2?1800000000:0;*trust=id==2?2:0;return true;}
meeting_upload_result meeting_runtime_upload(uint32_t id,bool has_time,int64_t recorded_at,uint8_t trust,const meeting_online_config *config)
{
    assert(config&&has_time==(id==2)&&recorded_at==(id==2?1800000000:0)&&trust==(id==2?2:0));seen[calls]=id;
    meeting_upload_result result=answers[calls++];if(simulate_removal&&result==MEETING_UPLOAD_CONFIRMED)removed[id]=true;
    if(drop_after&&calls==drop_after)online=0;return result;
}
int main(void)
{
    meeting_online_config config={.has_ip=has_ip,.https={.is_cancelled=cancelled,.time_valid=time_ok}};
    sc_file_entry_t scratch[3];meeting_worker_result r;
    online=0;r=meeting_worker_pass(&config,scratch,3);assert(!r.attempted&&!calls);
    online=1;clock_ok=0;r=meeting_worker_pass(&config,scratch,3);assert(!r.attempted&&!calls);
    clock_ok=1;cancelled_flag=1;r=meeting_worker_pass(&config,scratch,3);assert(!r.attempted&&!calls);
    cancelled_flag=0;answers[0]=MEETING_UPLOAD_CONFIRMED;answers[1]=MEETING_UPLOAD_FAILED;answers[2]=MEETING_UPLOAD_PAUSED;
    r=meeting_worker_pass(&config,scratch,3);
    assert(r.attempted==3&&r.confirmed==1&&r.failed==1&&r.paused);
    assert(seen[0]==1&&seen[1]==2&&seen[2]==3);
    calls=0;answers[0]=MEETING_UPLOAD_PAUSED;r=meeting_worker_pass(&config,scratch,3);
    assert(r.attempted==1&&r.paused&&calls==1);
    calls=0;online=1;drop_after=1;answers[0]=MEETING_UPLOAD_FAILED;
    r=meeting_worker_pass(&config,scratch,3);assert(r.attempted==1&&r.failed==1&&r.paused&&calls==1);
    calls=0;online=1;drop_after=0;
    answers[0]=answers[1]=answers[2]=MEETING_UPLOAD_CONFIRMED;
    r=meeting_worker_pass(&config,scratch,3);assert(r.attempted==3&&r.confirmed==3&&!r.paused);
    assert(seen[0]==1&&seen[1]==2&&seen[2]==3); /* reconnect retries all pending work */
    calls=0;missing_metadata=1;answers[0]=answers[1]=MEETING_UPLOAD_CONFIRMED;
    r=meeting_worker_pass(&config,scratch,3);assert(r.attempted==3&&r.confirmed==2&&r.failed==1&&calls==2&&seen[0]==1&&seen[1]==3);
    missing_metadata=0;
    calls=0;online=1;drop_after=0;empty_first=1;answers[0]=MEETING_UPLOAD_FAILED;answers[1]=MEETING_UPLOAD_CONFIRMED;
    r=meeting_worker_pass(&config,scratch,3);assert(r.attempted==2&&r.failed==1&&r.confirmed==1&&!r.paused);
    assert(seen[0]==2&&seen[1]==3); /* empty and failing records do not block later records */
    calls=0;empty_first=0;answers[0]=answers[1]=answers[2]=MEETING_UPLOAD_CONFIRMED;
    r=meeting_worker_pass(&config,scratch,1);assert(r.attempted==3&&r.confirmed==3&&!r.paused);
    assert(seen[0]==1&&seen[1]==2&&seen[2]==3); /* bounded pages remain oldest first */
    calls=0;simulate_removal=1;r=meeting_worker_pass(&config,scratch,2);
    assert(r.attempted==3&&r.confirmed==3&&sc_store_count()==0);
    assert(seen[0]==1&&seen[1]==2&&seen[2]==3); /* cleanup does not skip newer pages */
    return 0;
}
