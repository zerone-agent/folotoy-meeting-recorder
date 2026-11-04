#pragma once
#include "meeting_confirmation.h"
#include "nvs.h"
typedef struct { nvs_handle_t handle; uint32_t file_id; } meeting_confirmation_nvs;
bool meeting_confirmation_nvs_bind(meeting_confirmation_nvs *storage, meeting_confirmation_io *io);
