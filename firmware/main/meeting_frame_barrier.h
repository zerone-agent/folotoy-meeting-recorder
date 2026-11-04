#pragma once
#include <stdatomic.h>
#include <stdbool.h>
/* Producer registers before publishing to the queue; consumer completes only
 * after its write finishes. Pending therefore covers the receive/write gap. */
typedef struct { atomic_uint pending; atomic_uint lost; } meeting_frame_barrier;
void meeting_frame_barrier_begin(meeting_frame_barrier *barrier);
void meeting_frame_barrier_rejected(meeting_frame_barrier *barrier);
void meeting_frame_barrier_completed(meeting_frame_barrier *barrier);
bool meeting_frame_barrier_drained(const meeting_frame_barrier *barrier);
typedef void (*meeting_frame_wait_fn)(void *ctx);
/* Bound the wait. Failure means the caller must not close or delete audio. */
bool meeting_frame_barrier_wait(const meeting_frame_barrier *barrier,unsigned attempts,
                                meeting_frame_wait_fn wait,void *ctx);
unsigned meeting_frame_barrier_lost(const meeting_frame_barrier *barrier);
/* Call only after capture has stopped and all pending frames are complete. */
bool meeting_frame_barrier_reset_loss(meeting_frame_barrier *barrier);
