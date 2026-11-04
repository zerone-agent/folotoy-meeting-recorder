#pragma once
#include "meeting_time_anchor.h"
#include "nvs.h"
/* The caller owns and serializes the handle for namespace meeting_timekey. */
bool meeting_time_anchor_nvs_bind(nvs_handle_t *handle,meeting_time_anchor_io *out);
