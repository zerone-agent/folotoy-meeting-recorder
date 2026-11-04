#include "meeting_writer_signal.h"
#include <assert.h>
int main(void){
 meeting_writer_signal s={0};meeting_writer_event e;
 assert(!meeting_writer_signal_read(&s,0,&e));
 meeting_writer_signal_publish(&s,123,1);
 assert(meeting_writer_signal_read(&s,0,&e)&&e.sequence==2&&e.id==123&&e.reason==1);
 assert(!meeting_writer_signal_read(&s,e.sequence,&e));
 meeting_writer_signal_publish(&s,456,2);
 assert(meeting_writer_signal_read(&s,2,&e)&&e.sequence==4&&e.id==456&&e.reason==2);
 atomic_store(&s.sequence,5);assert(!meeting_writer_signal_read(&s,4,&e));
 return 0;
}
