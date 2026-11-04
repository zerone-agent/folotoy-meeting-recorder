#pragma once
#include "meeting_snapshot.h"
#include "nvs.h"
/* Caller owns handle and serializes operations for each file ID. Opening NVS
 * must not erase it; failures and invalid records must be surfaced. */
typedef struct { nvs_handle_t handle; uint32_t file_id; } meeting_snapshot_nvs;
bool meeting_snapshot_nvs_bind(meeting_snapshot_nvs *storage, meeting_snapshot_io *io);
