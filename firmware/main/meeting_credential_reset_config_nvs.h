#pragma once
#include "meeting_config_store_nvs.h"
#include "meeting_credential_identity.h"

/* The caller must stop and serialize networking/provisioning before use.
 * This adapter affects only meeting_config/active, never audio or clock NVS. */
typedef struct {
    nvs_handle_t config_handle;
    meeting_ca_validate validate_ca;
    void *ca_ctx;
} meeting_reset_config_nvs;
int meeting_reset_config_state(void *ctx,const meeting_reset_record *record);
bool meeting_reset_config_clear(void *ctx);
