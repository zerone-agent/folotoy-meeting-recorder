#pragma once
#include <stdbool.h>
#include <stdint.h>

/* The record contains identifiers only; never place a device or admin token in it. */
#define MEETING_RESET_RECORD_SIZE 480u
typedef enum { MEETING_RESET_PENDING=1, MEETING_RESET_LOCAL_CLEARED=2,
               MEETING_RESET_REVOKED=3 } meeting_reset_state;
typedef struct {
    char origin[201],device_id[101],credential_id[65],event_id[101];
    meeting_reset_state state;
} meeting_reset_record;
typedef struct {
    void *ctx;
    /* 0 present, 1 absent, -1 storage failure. */
    int (*read)(void *ctx,uint8_t out[MEETING_RESET_RECORD_SIZE]);
    bool (*write_sync)(void *ctx,const uint8_t data[MEETING_RESET_RECORD_SIZE]);
    /* 0 matching active credential, 1 already absent, -1 mismatch/error. */
    int (*credential_state)(void *ctx,const meeting_reset_record *record);
    /* Idempotent durable erase of network and device credentials only. */
    bool (*clear_credentials)(void *ctx);
} meeting_reset_io;
typedef enum {MEETING_RESET_OK,MEETING_RESET_NOT_FOUND,MEETING_RESET_CONFLICT,MEETING_RESET_ERROR} meeting_reset_result;

bool meeting_reset_encode(const meeting_reset_record *record,uint8_t out[MEETING_RESET_RECORD_SIZE]);
bool meeting_reset_decode(const uint8_t data[MEETING_RESET_RECORD_SIZE],meeting_reset_record *out);
/* Serialize calls with provisioning, upload and deletion. Identity must have
 * been obtained from the authenticated existing configuration. Repeated calls
 * with the same pending event recover after interruption. */
meeting_reset_result meeting_reset_begin(const meeting_reset_io *io,const meeting_reset_record *identity);
/* Caller verifies the matching service receipt through an independently
 * authenticated administrator channel before setting receipt_verified. */
meeting_reset_result meeting_reset_ack(const meeting_reset_io *io,const meeting_reset_record *identity,bool receipt_verified);
