#pragma once
#include "meeting_deletion.h"
#include "nvs.h"
/* Caller serializes operations for this file. Never erase NVS on error. */
typedef struct {nvs_handle_t handle;uint32_t file_id;} meeting_deletion_nvs;
bool meeting_deletion_nvs_bind(meeting_deletion_nvs *storage,meeting_deletion_io *io);
