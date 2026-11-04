#pragma once
#include "meeting_deletion.h"

/* Caller must hold exclusive ownership for this recording and drain in-flight
 * upload requests before invoking this workflow. All effects are idempotent. */
typedef struct {
    void *ctx;
    /* Persist and verify any manifest/identity evidence needed to reconcile a
     * committed server response before deleting the only local audio copy.
     * Must be idempotent and survive reboot, including after local deletion. */
    bool (*ensure_remote_proof)(void *ctx,const meeting_deletion *intent);
    bool (*remove_local)(void *ctx,const meeting_deletion *intent);
    /* 0 means offline/unknown, 1 cancelled, 2 committed, -1 error. A terminal
     * result must be authenticated and bound to intent->meeting_id. */
    int (*cancel_remote)(void *ctx,const meeting_deletion *intent);
} meeting_delete_workflow_io;
typedef enum {MEETING_DELETE_DONE,MEETING_DELETE_PENDING,MEETING_DELETE_ERROR} meeting_delete_result;
/* Intent must already be durable. A restart can call this again even when the
 * local file has disappeared or a previous response was lost. */
meeting_delete_result meeting_delete_resume(const meeting_deletion_io *storage,const meeting_delete_workflow_io *io);
