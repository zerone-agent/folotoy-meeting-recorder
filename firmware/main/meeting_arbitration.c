#include "meeting_arbitration.h"
enum { IDLE, UPLOAD, UPLOAD_STOP, RECORD_PENDING, RECORDING, MAINTENANCE, MAINT_RECORD_PENDING };
void meeting_arbitration_init(meeting_arbitration *a){atomic_init(&a->state,IDLE);}
bool meeting_arbitration_upload_begin(meeting_arbitration *a){unsigned expected=IDLE;return atomic_compare_exchange_strong(&a->state,&expected,UPLOAD);}
bool meeting_arbitration_upload_cancelled(const meeting_arbitration *a){return atomic_load(&a->state)!=UPLOAD;}
void meeting_arbitration_upload_end(meeting_arbitration *a){unsigned old=atomic_load(&a->state);for(;;){unsigned next;if(old==UPLOAD)next=IDLE;else if(old==UPLOAD_STOP)next=RECORD_PENDING;else return;if(atomic_compare_exchange_weak(&a->state,&old,next))return;}}
bool meeting_arbitration_record_request(meeting_arbitration *a){unsigned old=atomic_load(&a->state);for(;;){unsigned next;if(old==IDLE)next=RECORD_PENDING;else if(old==UPLOAD)next=UPLOAD_STOP;else if(old==MAINTENANCE)next=MAINT_RECORD_PENDING;else return old==UPLOAD_STOP||old==RECORD_PENDING||old==MAINT_RECORD_PENDING;if(atomic_compare_exchange_weak(&a->state,&old,next))return true;}}
bool meeting_arbitration_record_acquire(meeting_arbitration *a){unsigned expected=RECORD_PENDING;return atomic_compare_exchange_strong(&a->state,&expected,RECORDING);}
bool meeting_arbitration_record_cancel(meeting_arbitration *a){unsigned old=atomic_load(&a->state);for(;;){unsigned next;if(old==RECORD_PENDING)next=IDLE;else if(old==UPLOAD_STOP)next=UPLOAD;else if(old==MAINT_RECORD_PENDING)next=MAINTENANCE;else return false;if(atomic_compare_exchange_weak(&a->state,&old,next))return true;}}
bool meeting_arbitration_record_end(meeting_arbitration *a){unsigned expected=RECORDING;return atomic_compare_exchange_strong(&a->state,&expected,IDLE);}

bool meeting_arbitration_maintenance_acquire(meeting_arbitration *a){unsigned expected=IDLE;return atomic_compare_exchange_strong(&a->state,&expected,MAINTENANCE);}
bool meeting_arbitration_maintenance_release(meeting_arbitration *a){unsigned old=atomic_load(&a->state);for(;;){unsigned next;if(old==MAINTENANCE)next=IDLE;else if(old==MAINT_RECORD_PENDING)next=RECORD_PENDING;else return false;if(atomic_compare_exchange_weak(&a->state,&old,next))return true;}}
