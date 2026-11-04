#pragma once
#include "meeting_credential_reset.h"
#include "nvs.h"

/* State and callback contexts must outlive the bound interface. The caller
 * opens a dedicated read/write namespace and serializes all reset operations. */
typedef struct {
    nvs_handle_t handle;
    void *credential_ctx;
    int (*credential_state)(void *ctx,const meeting_reset_record *record);
    bool (*clear_credentials)(void *ctx);
} meeting_reset_nvs;
bool meeting_reset_nvs_bind(meeting_reset_nvs *state,meeting_reset_io *out);
