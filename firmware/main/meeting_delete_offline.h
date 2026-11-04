#pragma once
#include "esp_err.h"
/* Start once after runtime initialization. Does not require Wi-Fi or clock;
 * remote cancellation remains pending for the network worker. */
esp_err_t meeting_delete_offline_start(void);
/* Retry a failed start or recovery pass without requiring network access. */
esp_err_t meeting_delete_offline_retry(void);
