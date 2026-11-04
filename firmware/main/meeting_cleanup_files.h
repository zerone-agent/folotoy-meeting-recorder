#pragma once
#include "meeting_cleanup.h"
#include "meeting_scr1_file.h"

/* Caller owns paths and serializes access for the full cleanup. The close
 * callback must close every reader of both files; space callback must perform
 * a fresh filesystem usage query and fail on I/O errors. */
typedef struct {
    const char *audio_path,*index_path;
    meeting_cancel_fn cancelled;
    bool (*close_readers)(void *ctx);
    bool (*read_space)(void *ctx);
    void *ctx;
} meeting_cleanup_files;
bool meeting_cleanup_files_bind(meeting_cleanup_files *files,meeting_cleanup_io *io);
