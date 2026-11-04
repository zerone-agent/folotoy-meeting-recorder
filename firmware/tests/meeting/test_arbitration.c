#include "meeting_arbitration.h"
#include <assert.h>
int main(void){meeting_arbitration a;meeting_arbitration_init(&a);assert(meeting_arbitration_upload_begin(&a));assert(meeting_arbitration_record_request(&a));assert(meeting_arbitration_upload_cancelled(&a));assert(!meeting_arbitration_record_acquire(&a));assert(!meeting_arbitration_upload_begin(&a));meeting_arbitration_upload_end(&a);assert(meeting_arbitration_record_acquire(&a));assert(!meeting_arbitration_upload_begin(&a));assert(meeting_arbitration_record_end(&a));assert(meeting_arbitration_upload_begin(&a));assert(meeting_arbitration_record_request(&a));assert(meeting_arbitration_record_cancel(&a));assert(!meeting_arbitration_upload_cancelled(&a));meeting_arbitration_upload_end(&a);assert(meeting_arbitration_record_request(&a));assert(meeting_arbitration_record_acquire(&a));assert(meeting_arbitration_record_end(&a));
 assert(meeting_arbitration_maintenance_acquire(&a));assert(!meeting_arbitration_upload_begin(&a));
 assert(meeting_arbitration_record_request(&a));assert(!meeting_arbitration_record_acquire(&a));
 assert(meeting_arbitration_maintenance_release(&a));assert(meeting_arbitration_record_acquire(&a));assert(meeting_arbitration_record_end(&a));
 assert(meeting_arbitration_record_request(&a));assert(!meeting_arbitration_maintenance_acquire(&a));assert(meeting_arbitration_record_cancel(&a));
 assert(meeting_arbitration_maintenance_acquire(&a));assert(meeting_arbitration_record_request(&a));assert(meeting_arbitration_record_cancel(&a));assert(meeting_arbitration_maintenance_release(&a));
 assert(meeting_arbitration_upload_begin(&a));assert(!meeting_arbitration_maintenance_acquire(&a));meeting_arbitration_upload_end(&a);
 return 0;}
