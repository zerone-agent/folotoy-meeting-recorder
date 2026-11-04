#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MEETING_SNAPSHOT_SIZE 88u
#define MEETING_PROJECTION_V1 1u
/* Explicitly encoded state; never serialize native structs. The caller must
 * supply durable atomic slot storage and verify the immutable source hash. */
typedef struct {
    uint64_t generation;
    uint64_t valid_end;
    uint32_t frames;
    uint8_t meeting_id[16];
    uint8_t source_sha256[32];
    uint8_t reason;
} meeting_snapshot;
bool meeting_snapshot_encode(const meeting_snapshot *value, uint8_t out[MEETING_SNAPSHOT_SIZE]);
bool meeting_snapshot_decode(const uint8_t in[MEETING_SNAPSHOT_SIZE], meeting_snapshot *out);

/* Slot reads: 0=present, 1=never written, -1=I/O error. Any invalid slot
 * alongside a valid one is DEGRADED and cannot authorize destructive actions. */
typedef struct {
    void *ctx;
    int (*read)(void *ctx, unsigned slot, uint8_t out[MEETING_SNAPSHOT_SIZE]);
    bool (*write_sync)(void *ctx, unsigned slot, const uint8_t data[MEETING_SNAPSHOT_SIZE]);
} meeting_snapshot_io;
typedef enum { MEETING_SNAPSHOT_OK, MEETING_SNAPSHOT_EMPTY, MEETING_SNAPSHOT_DEGRADED, MEETING_SNAPSHOT_ERROR } meeting_snapshot_status;
meeting_snapshot_status meeting_snapshot_load(const meeting_snapshot_io *io, meeting_snapshot *out, unsigned *slot);
/* Returns false for ambiguous write failures; re-read and reconcile before use. */
bool meeting_snapshot_commit(const meeting_snapshot_io *io, const meeting_snapshot *next);
