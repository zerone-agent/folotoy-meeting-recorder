#pragma once
#include "meeting_deletion_nvs.h"
typedef struct {meeting_deletion_nvs nvs;bool opened;} meeting_deletion_store;
/* The dedicated namespace must never be confused with upload confirmations. */
bool meeting_deletion_store_open(meeting_deletion_store *store,uint32_t file_id,meeting_deletion_io *io);
void meeting_deletion_store_close(meeting_deletion_store *store);
/* Enumerates ids in ascending order; both slots of one id count only once.
 * Unexpected entries and storage errors are failures, not an empty queue. */
typedef enum {MEETING_DELETION_NEXT_FOUND,MEETING_DELETION_NEXT_END,MEETING_DELETION_NEXT_ERROR} meeting_deletion_next_status;
meeting_deletion_next_status meeting_deletion_store_next(uint32_t after,uint32_t *file_id);
/* False includes present intent, damaged data and I/O errors. The caller must
 * serialize this check with intent creation and upload ownership. */
bool meeting_deletion_store_upload_permitted(uint32_t file_id);
