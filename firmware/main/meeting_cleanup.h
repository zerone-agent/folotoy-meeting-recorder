#pragma once
#include "meeting_confirmation.h"

/* Caller serializes recording, readers, manual deletion and cleanup for this
 * file, and supplies the verified durable snapshot and file/meeting mapping.
 * All callbacks are task-only. Missing and I/O error must be distinguished. */
typedef struct {
    void *ctx;
    bool (*cancelled)(void *ctx);
    int (*audio_exists)(void *ctx); /* 0 present, 1 absent, -1 error */
    bool (*verify_audio)(void *ctx,const meeting_confirmation *receipt,const meeting_snapshot *snapshot);
    bool (*close_readers)(void *ctx);
    bool (*remove_audio)(void *ctx); /* idempotent, including absent file */
    bool (*remove_index)(void *ctx); /* idempotent, including absent index */
    bool (*verify_absent_and_space)(void *ctx);
} meeting_cleanup_io;
typedef enum { MEETING_CLEANUP_DONE, MEETING_CLEANUP_PAUSED, MEETING_CLEANUP_ERROR } meeting_cleanup_result;
/* The intent store is a different durable namespace from the receipt. Intent
 * remains after completion so a restart can safely repeat cleanup. */
meeting_cleanup_result meeting_cleanup_run(uint32_t file_id,const meeting_snapshot *snapshot,
    const meeting_confirmation_io *receipts,const meeting_confirmation_io *intents,const meeting_cleanup_io *io);
