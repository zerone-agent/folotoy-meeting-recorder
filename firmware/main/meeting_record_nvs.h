#pragma once
#include "meeting_record.h"
#include "nvs.h"
typedef enum { MEETING_RECORD_OK, MEETING_RECORD_MISSING, MEETING_RECORD_ERROR } meeting_record_status;
/* Caller serializes access; no operation formats or erases NVS. */
meeting_record_status meeting_record_nvs_read(nvs_handle_t nvs, uint32_t file_id, meeting_record *out);
/* Only creates a missing identity, or verifies an identical retry. */
bool meeting_record_nvs_create(nvs_handle_t nvs,const meeting_record *record);
/* Records a once-only intent before closing; ambiguous failures require reread. */
bool meeting_record_nvs_end_intent(nvs_handle_t nvs,uint32_t file_id,uint8_t intent);
