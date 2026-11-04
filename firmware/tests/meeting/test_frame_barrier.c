#include "meeting_frame_barrier.h"
#include <assert.h>
typedef struct {meeting_frame_barrier *barrier;unsigned ticks,finish_at;} waiter;
static void wait_once(void *ctx){waiter *w=ctx;if(++w->ticks==w->finish_at)meeting_frame_barrier_completed(w->barrier);}
int main(void){
 meeting_frame_barrier b={0};assert(meeting_frame_barrier_drained(&b));
 meeting_frame_barrier_begin(&b);assert(!meeting_frame_barrier_drained(&b));
 /* Producer has published; consumer removed the frame but has not written. */
 assert(!meeting_frame_barrier_drained(&b));assert(!meeting_frame_barrier_reset_loss(&b));
 waiter w={.barrier=&b,.finish_at=3};
 assert(!meeting_frame_barrier_wait(&b,2,wait_once,&w));
 assert(!meeting_frame_barrier_drained(&b));
 assert(meeting_frame_barrier_wait(&b,1,wait_once,&w)&&meeting_frame_barrier_drained(&b));
 meeting_frame_barrier_begin(&b);meeting_frame_barrier_rejected(&b);
 assert(meeting_frame_barrier_drained(&b)&&meeting_frame_barrier_lost(&b)==1);
 assert(meeting_frame_barrier_reset_loss(&b)&&meeting_frame_barrier_lost(&b)==0);
 return 0;
}
