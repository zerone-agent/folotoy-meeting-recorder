#include "meeting_reset_coordinator.h"
#include <string.h>

typedef struct {const meeting_reset_coordinator_io *io;const meeting_reset_record *identity;} guarded_io;
static bool authorized(const guarded_io *g)
{return g->io->session_valid(g->io->ctx)&&g->io->admin_authorized(g->io->ctx,g->identity);}
static int guarded_read(void *ctx,uint8_t out[MEETING_RESET_RECORD_SIZE])
{guarded_io *g=ctx;return authorized(g)?g->io->storage.read(g->io->storage.ctx,out):-1;}
static bool guarded_write(void *ctx,const uint8_t data[MEETING_RESET_RECORD_SIZE])
{guarded_io *g=ctx;return authorized(g)&&g->io->storage.write_sync(g->io->storage.ctx,data);}
static int guarded_state(void *ctx,const meeting_reset_record *record)
{guarded_io *g=ctx;return authorized(g)?g->io->storage.credential_state(g->io->storage.ctx,record):-1;}
static bool guarded_clear(void *ctx)
{guarded_io *g=ctx;return authorized(g)&&g->io->storage.clear_credentials(g->io->storage.ctx);}
static bool valid_io(const meeting_reset_coordinator_io *io)
{return io&&io->session_valid&&io->admin_authorized&&io->network_stop&&io->network_stopped&&
        io->maintenance_acquire&&io->maintenance_release&&io->storage.read&&io->storage.write_sync&&
        io->storage.credential_state&&io->storage.clear_credentials;}
meeting_reset_coord_result meeting_reset_coordinator_start(meeting_reset_coordinator *state,
                        const meeting_reset_coordinator_io *io,const meeting_reset_record *identity)
{
    uint8_t encoded[MEETING_RESET_RECORD_SIZE];
    if(!state||state->active||!valid_io(io)||!identity||identity->state!=MEETING_RESET_PENDING||
       !meeting_reset_encode(identity,encoded))return MEETING_RESET_COORD_ERROR;
    guarded_io guard={io,identity};if(!authorized(&guard))return MEETING_RESET_COORD_AUTH_LOST;
    if(!io->network_stop(io->ctx))return MEETING_RESET_COORD_ERROR;
    state->identity=*identity;state->active=true;return MEETING_RESET_COORD_WAIT_NETWORK;
}
meeting_reset_coord_result meeting_reset_coordinator_poll(meeting_reset_coordinator *state,
                        const meeting_reset_coordinator_io *io)
{
    if(!state||!state->active||!valid_io(io))return MEETING_RESET_COORD_ERROR;
    guarded_io guard={io,&state->identity};
    if(!authorized(&guard)){state->active=false;return MEETING_RESET_COORD_AUTH_LOST;}
    if(!io->network_stopped(io->ctx))return MEETING_RESET_COORD_WAIT_NETWORK;
    if(!io->maintenance_acquire(io->ctx))return MEETING_RESET_COORD_WAIT_RECORDING;
    meeting_reset_io guarded={&guard,guarded_read,guarded_write,guarded_state,guarded_clear};
    meeting_reset_result result=meeting_reset_begin(&guarded,&state->identity);
    bool release_ok=io->maintenance_release(io->ctx);
    bool auth_ok=authorized(&guard);
    state->active=false;
    if(!release_ok)return MEETING_RESET_COORD_ERROR;
    if(!auth_ok)return MEETING_RESET_COORD_AUTH_LOST;
    return result==MEETING_RESET_OK?MEETING_RESET_COORD_DONE:MEETING_RESET_COORD_ERROR;
}
