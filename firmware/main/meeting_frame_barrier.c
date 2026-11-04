#include "meeting_frame_barrier.h"
void meeting_frame_barrier_begin(meeting_frame_barrier *b){atomic_fetch_add(&b->pending,1);}
void meeting_frame_barrier_rejected(meeting_frame_barrier *b){atomic_fetch_add(&b->lost,1);atomic_fetch_sub(&b->pending,1);}
void meeting_frame_barrier_completed(meeting_frame_barrier *b){atomic_fetch_sub(&b->pending,1);}
bool meeting_frame_barrier_drained(const meeting_frame_barrier *b){return atomic_load(&b->pending)==0;}
bool meeting_frame_barrier_wait(const meeting_frame_barrier *b,unsigned attempts,meeting_frame_wait_fn wait,void *ctx)
{
    if(!b||!wait)return false;
    for(unsigned i=0;i<attempts&&!meeting_frame_barrier_drained(b);i++)wait(ctx);
    return meeting_frame_barrier_drained(b);
}
unsigned meeting_frame_barrier_lost(const meeting_frame_barrier *b){return atomic_load(&b->lost);}
bool meeting_frame_barrier_reset_loss(meeting_frame_barrier *b){if(!meeting_frame_barrier_drained(b))return false;atomic_store(&b->lost,0);return true;}
