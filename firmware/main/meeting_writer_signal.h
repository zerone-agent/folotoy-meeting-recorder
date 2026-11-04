#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
/* One writer publishes, one coordinator consumes. Updates do not allocate or
 * depend on a bounded event queue. Sequence guards the ID and reason pair. */
typedef struct {atomic_uint sequence,id,reason;} meeting_writer_signal;
typedef struct {uint32_t sequence,id,reason;} meeting_writer_event;
void meeting_writer_signal_publish(meeting_writer_signal *signal,uint32_t id,uint32_t reason);
bool meeting_writer_signal_read(const meeting_writer_signal *signal,uint32_t after,meeting_writer_event *event);
