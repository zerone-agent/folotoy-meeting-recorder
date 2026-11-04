#pragma once
#include "meeting_arbitration.h"
#include "meeting_store_upload.h"
#include "meeting_cleanup.h"
#include "meeting_delete_ui.h"
#include "esp_err.h"

/* Initialize before starting the core or network tasks. The core owns recording
 * transitions; network workers hold upload ownership through source release. */
void meeting_runtime_init(void);
bool meeting_runtime_record_request(void);
bool meeting_runtime_record_acquire(void);
bool meeting_runtime_record_cancel(void);
bool meeting_runtime_record_end(void);
bool meeting_runtime_maintenance_acquire(void);
bool meeting_runtime_maintenance_release(void);
meeting_upload_result meeting_runtime_upload(uint32_t file_id, bool has_time, int64_t recorded_at,
                                             uint8_t time_trust, const meeting_online_config *config);
meeting_cleanup_result meeting_runtime_resume_cleanup(void);
typedef struct {unsigned attempted,completed,failed;bool paused;} meeting_delete_pass_result;
/* Local steps can run offline; remote cancellation waits for trusted HTTPS. */
meeting_delete_pass_result meeting_runtime_resume_deletions(const meeting_online_config *config);
/* Task-only; returns UNKNOWN if recording or an upload currently owns the
 * device. A later confirmation must be rechecked while creating the intent. */
meeting_delete_visibility meeting_runtime_review_delete(uint32_t file_id);
esp_err_t meeting_runtime_begin_delete(uint32_t file_id,meeting_delete_visibility reviewed);
