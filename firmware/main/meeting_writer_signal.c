#include "meeting_writer_signal.h"
void meeting_writer_signal_publish(meeting_writer_signal *s,uint32_t id,uint32_t reason)
{
    atomic_fetch_add(&s->sequence,1);
    atomic_store(&s->id,id);atomic_store(&s->reason,reason);
    atomic_fetch_add(&s->sequence,1);
}
bool meeting_writer_signal_read(const meeting_writer_signal *s,uint32_t after,meeting_writer_event *event)
{
    if(!s||!event)return false;
    uint32_t before=atomic_load(&s->sequence);
    if(!before||before==after||(before&1u))return false;
    meeting_writer_event value={before,atomic_load(&s->id),atomic_load(&s->reason)};
    if(atomic_load(&s->sequence)!=before)return false;
    *event=value;return true;
}
