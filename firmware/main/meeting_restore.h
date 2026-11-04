#pragma once
#include "esp_err.h"
/* Start from previously committed configuration, time anchor and floor.
 * Must be called while the core owns the idle radio, before legacy BLE. */
esp_err_t meeting_restore_start(void);
