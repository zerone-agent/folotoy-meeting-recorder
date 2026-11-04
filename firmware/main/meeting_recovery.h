#pragma once
#include "meeting_snapshot.h"
#include "meeting_scr1_file.h"
#include "meeting_seal.h"
#include <stdio.h>
/* Requires exclusive access to source and serialized I/O for the entire call.
 * Caller provides a cryptographically random stable UUID on first creation.
 * Never modifies source. Existing snapshot is verified, never overwritten. */
typedef enum { MEETING_RECOVERY_OK, MEETING_RECOVERY_STORAGE_ERROR, MEETING_RECOVERY_SOURCE_ERROR, MEETING_RECOVERY_ID_CONFLICT } meeting_recovery_status;
meeting_recovery_status meeting_recovery_prepare(FILE *source, const meeting_snapshot_io *io,
                                                 const uint8_t meeting_id[16], meeting_snapshot *out);
meeting_recovery_status meeting_recovery_prepare_cancel(FILE *source, const meeting_snapshot_io *io,
                                                 const uint8_t meeting_id[16], meeting_snapshot *out,
                                                 meeting_cancel_fn cancelled, void *ctx);

/* Closed recordings require a previously persisted, authoritative end reason. */
meeting_recovery_status meeting_closed_prepare(FILE *source, const meeting_snapshot_io *io,
                                               const uint8_t meeting_id[16], uint8_t reason,
                                               meeting_snapshot *out);
meeting_recovery_status meeting_closed_prepare_cancel(FILE *source, const meeting_snapshot_io *io,
                                               const uint8_t meeting_id[16], uint8_t reason,
                                               meeting_snapshot *out, meeting_cancel_fn cancelled, void *ctx);
/* Validates an open-header source against a separately committed end proof. */
meeting_recovery_status meeting_sealed_prepare_cancel(FILE *source,const meeting_snapshot_io *io,
                                               const meeting_seal *seal,meeting_snapshot *out,
                                               meeting_cancel_fn cancelled,void *ctx);
