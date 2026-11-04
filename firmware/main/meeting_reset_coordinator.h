#pragma once
#include "meeting_credential_reset.h"

/* The control channel and administrator authorization are independent gates.
 * A BLE session or device credential alone must never satisfy both gates. */
typedef struct {
    void *ctx;
    bool (*session_valid)(void *ctx);
    bool (*admin_authorized)(void *ctx,const meeting_reset_record *identity);
    bool (*network_stop)(void *ctx);
    bool (*network_stopped)(void *ctx);
    bool (*maintenance_acquire)(void *ctx);
    bool (*maintenance_release)(void *ctx);
    meeting_reset_io storage;
} meeting_reset_coordinator_io;
typedef struct {meeting_reset_record identity;bool active;} meeting_reset_coordinator;
typedef enum {MEETING_RESET_COORD_DONE,MEETING_RESET_COORD_WAIT_NETWORK,
              MEETING_RESET_COORD_WAIT_RECORDING,MEETING_RESET_COORD_AUTH_LOST,
              MEETING_RESET_COORD_ERROR} meeting_reset_coord_result;
/* Caller serializes calls and must block new provisioning while active. */
meeting_reset_coord_result meeting_reset_coordinator_start(meeting_reset_coordinator *state,
                        const meeting_reset_coordinator_io *io,const meeting_reset_record *identity);
meeting_reset_coord_result meeting_reset_coordinator_poll(meeting_reset_coordinator *state,
                        const meeting_reset_coordinator_io *io);
