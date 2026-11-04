#pragma once
#include "meeting_snapshot.h"
#include "meeting_scr1_file.h"
/* Caller must hold exclusive access to the source through hashing and commit. */
bool meeting_snapshot_source_hash(FILE *file, uint8_t digest[32]);
bool meeting_snapshot_source_hash_cancel(FILE *file, uint8_t digest[32], meeting_cancel_fn cancelled, void *ctx);
bool meeting_snapshot_matches_file(FILE *file, const meeting_snapshot *snapshot);
bool meeting_snapshot_matches_file_cancel(FILE *file, const meeting_snapshot *snapshot, meeting_cancel_fn cancelled, void *ctx);
/* Caller holds exclusive access and has verified snapshot identity. Hashes the
 * projected byte range with fixed memory; never hashes raw header secrets. */
bool meeting_projection_digest(FILE *file, const meeting_scr1_scan_result *scan,
                               uint64_t offset, uint64_t length, uint8_t digest[32]);
bool meeting_projection_digest_cancel(FILE *file, const meeting_scr1_scan_result *scan,
                                      uint64_t offset, uint64_t length, uint8_t digest[32], meeting_cancel_fn cancelled, void *ctx);
