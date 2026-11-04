#include "meeting_clock.h"
#include <limits.h>
#include <string.h>

#define EARLIEST_EPOCH 1735689600LL
#define LATEST_EPOCH 4102444800LL
#define MAX_DRIFT_SECONDS 5LL
void meeting_clock_reset(meeting_clock *clock){if(clock)memset(clock,0,sizeof(*clock));}
bool meeting_clock_bootstrap(meeting_clock *clock,const meeting_clock_io *io,bool authenticated,
                             bool user_confirmed,bool allow_initial_floor,int64_t epoch,uint64_t uptime_ms)
{
    if(!clock)return false;
    meeting_clock_reset(clock);
    if(!io||!io->read_floor||!io->commit_floor||!io->set_wall_time||!authenticated||!user_confirmed||
       epoch<EARLIEST_EPOCH||epoch>LATEST_EPOCH)return false;
    int64_t floor=0;int status=io->read_floor(io->ctx,&floor);
    if(status<0||status>1||(status==1&&!allow_initial_floor)||
       (status==0&&(floor<EARLIEST_EPOCH||floor>LATEST_EPOCH||epoch<floor)))return false;
    if(!io->commit_floor(io->ctx,epoch)||!io->set_wall_time(io->ctx,epoch))return false;
    *clock=(meeting_clock){.trusted=true,.epoch_at_sync=epoch,.uptime_at_sync_ms=uptime_ms,.persisted_floor=epoch};
    return true;
}
bool meeting_clock_valid(meeting_clock *clock,int64_t wall_epoch,uint64_t uptime_ms)
{
    if(!clock||!clock->trusted)return false;
    if(uptime_ms<clock->uptime_at_sync_ms){clock->trusted=false;return false;}
    uint64_t elapsed=(uptime_ms-clock->uptime_at_sync_ms)/1000;
    if(elapsed>(uint64_t)(LATEST_EPOCH-clock->epoch_at_sync)){clock->trusted=false;return false;}
    int64_t expected=clock->epoch_at_sync+(int64_t)elapsed;
    if(wall_epoch<EARLIEST_EPOCH||wall_epoch>LATEST_EPOCH||wall_epoch<clock->persisted_floor||
       wall_epoch<expected-MAX_DRIFT_SECONDS||wall_epoch>expected+MAX_DRIFT_SECONDS){clock->trusted=false;return false;}
    return true;
}
bool meeting_clock_checkpoint(meeting_clock *clock,const meeting_clock_io *io,int64_t wall_epoch,uint64_t uptime_ms)
{
    if(!io||!io->commit_floor||!meeting_clock_valid(clock,wall_epoch,uptime_ms))return false;
    if(wall_epoch<=clock->persisted_floor)return true;
    if(!io->commit_floor(io->ctx,wall_epoch)){clock->trusted=false;return false;}
    clock->persisted_floor=wall_epoch;return true;
}
