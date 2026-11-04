#pragma once
#include "meeting_clock.h"
#include "nvs.h"
/* Caller owns an open read/write handle to a dedicated namespace and
 * serializes provisioning/checkpoint operations. Never erase on failure. */
bool meeting_clock_nvs_bind(nvs_handle_t *handle, meeting_clock_io *io);
