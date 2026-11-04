#pragma once
#include "meeting_online_session.h"

/* Pins the source until the network session has completely exited. A confirmed
 * result alone does not authorize deleting the local recording. */
meeting_upload_result meeting_store_upload(uint32_t file_id, bool has_time,
                                           int64_t recorded_at, uint8_t time_trust,
                                           const meeting_online_config *config);
