#pragma once
#include <stdatomic.h>
#include <stdbool.h>
/* Lock-free state shared by the recording task and upload worker. A recording
 * request persists until taken or explicitly cancelled. No I/O in these calls. */
typedef struct { atomic_uint state; } meeting_arbitration;
void meeting_arbitration_init(meeting_arbitration *a);
bool meeting_arbitration_upload_begin(meeting_arbitration *a);
bool meeting_arbitration_upload_cancelled(const meeting_arbitration *a);
void meeting_arbitration_upload_end(meeting_arbitration *a);
bool meeting_arbitration_record_request(meeting_arbitration *a);
bool meeting_arbitration_record_acquire(meeting_arbitration *a);
bool meeting_arbitration_record_cancel(meeting_arbitration *a);
bool meeting_arbitration_record_end(meeting_arbitration *a);
/* A short maintenance transaction can start only while idle. A recording
 * request arriving during it is retained and becomes pending on release. */
bool meeting_arbitration_maintenance_acquire(meeting_arbitration *a);
bool meeting_arbitration_maintenance_release(meeting_arbitration *a);
