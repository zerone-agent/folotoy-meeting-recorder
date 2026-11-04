#pragma once
#include "meeting_confirmation.h"
#include "meeting_upload.h"
/* Store MUST use a namespace separate from server receipts. This is expected
 * content only and never proves server confirmation or authorizes cleanup. */
bool meeting_delete_proof_prepare(uint32_t file_id,FILE *source,const meeting_snapshot *snapshot,
    bool has_time,int64_t recorded_at,uint8_t time_trust,const meeting_confirmation_io *store,
    meeting_cancel_fn cancelled,void *ctx);
/* Source may already be removed; immutable snapshot and expected manifest
 * must be retained until the remote cancellation reaches a terminal state. */
bool meeting_delete_proof_load(uint32_t file_id,const meeting_snapshot *snapshot,
    const meeting_confirmation_io *store,meeting_manifest *expected);
