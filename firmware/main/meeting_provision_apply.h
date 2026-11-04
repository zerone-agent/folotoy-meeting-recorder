#pragma once
#include "meeting_clock.h"
#include "meeting_config_store.h"

typedef bool (*meeting_session_valid)(void *ctx);
typedef enum { MEETING_PROVISION_APPLIED, MEETING_PROVISION_REJECTED,
               MEETING_PROVISION_CLOCK_FAILED, MEETING_PROVISION_STORE_FAILED,
               MEETING_PROVISION_SESSION_LOST } meeting_provision_result;
/* Task-only. session_valid must identify the exact authenticated Security 2
 * session; confirmed values are those shown to and accepted by the user.
 * allow_initial_floor is permitted only after explicit supervised on-device
 * initial-clock confirmation; this does not authorize storage formatting.
 * On failure the clock is untrusted, and no network worker may be started. */
meeting_provision_result meeting_provision_apply(const uint8_t *json,size_t length,
    const uint8_t confirmed_ca_sha256[32],int64_t confirmed_utc,bool user_confirmed,
    meeting_session_valid session_valid,void *session_ctx,bool allow_initial_floor,uint64_t received_uptime_ms,uint64_t uptime_ms,
    const meeting_clock_io *clock_io,meeting_clock *clock,const meeting_config_store_io *store_io,
    meeting_ca_validate validate_ca,void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *out);
