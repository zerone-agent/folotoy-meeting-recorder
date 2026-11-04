#pragma once
#include "meeting_delete_ui.h"
#include "esp_err.h"
#include <stdint.h>
typedef enum {MEETING_DELETE_JOB_REVIEW,MEETING_DELETE_JOB_BEGIN} meeting_delete_job_kind;
typedef struct {uint32_t token,file_id;meeting_delete_job_kind kind;meeting_delete_visibility visibility;esp_err_t result;} meeting_delete_job_result;
typedef void (*meeting_delete_job_done)(const meeting_delete_job_result *result,void *ctx);
/* One job at a time, off the UI/core thread. Caller treats review tokens as an
 * epoch. BEGIN may persist an intent once submitted and must not be presented
 * as cancelled merely because its result became stale. */
esp_err_t meeting_delete_job_start(uint32_t token,uint32_t file_id,meeting_delete_job_kind kind,
                                   meeting_delete_visibility reviewed,meeting_delete_job_done done,void *ctx);
