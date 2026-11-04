#pragma once
#include "meeting_online_session.h"
#include "esp_err.h"
/* Configuration, callback contexts and strings must remain valid for the
 * device lifetime. Call only after secure provisioning and runtime init. */
esp_err_t meeting_worker_start(const meeting_online_config *config);
