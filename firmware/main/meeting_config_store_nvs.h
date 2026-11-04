#pragma once
#include "meeting_config_store.h"
#include "nvs.h"
/* Caller opens a dedicated read/write NVS namespace and serializes access. */
bool meeting_config_store_nvs_bind(nvs_handle_t *handle,meeting_config_store_io *io);
