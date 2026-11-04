#pragma once
#include "meeting_receipt.h"
#include "meeting_snapshot.h"
#include <stddef.h>

/* A durable receipt is bound to both the uploaded manifest and the local
 * immutable source. It is not itself an instruction to remove a file. */
#define MEETING_CONFIRMATION_SIZE 140u
typedef struct {
    uint32_t file_id;
    uint8_t source_sha256[32];
    meeting_manifest manifest;
} meeting_confirmation;
typedef struct {
    void *ctx;
    /* read: 0 present, 1 absent, negative on storage error */
    int (*read)(void *ctx, uint8_t out[MEETING_CONFIRMATION_SIZE]);
    bool (*write_sync)(void *ctx, const uint8_t data[MEETING_CONFIRMATION_SIZE]);
} meeting_confirmation_io;
typedef enum { MEETING_CONFIRMATION_OK, MEETING_CONFIRMATION_EMPTY,
               MEETING_CONFIRMATION_ERROR } meeting_confirmation_status;
bool meeting_confirmation_encode(const meeting_confirmation *value, uint8_t out[MEETING_CONFIRMATION_SIZE]);
bool meeting_confirmation_decode(const uint8_t data[MEETING_CONFIRMATION_SIZE], meeting_confirmation *value);
bool meeting_confirmation_matches_snapshot(const meeting_confirmation *value, const meeting_snapshot *snapshot);
meeting_confirmation_status meeting_confirmation_load(const meeting_confirmation_io *io, meeting_confirmation *out);
/* Requires an atomic durable backend. A failed or ambiguous write never grants
 * permission to clean up; a later invocation must read and reconcile it. */
bool meeting_confirmation_commit(const meeting_confirmation_io *io, const meeting_confirmation *value);
