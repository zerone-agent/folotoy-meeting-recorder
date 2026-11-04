#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MEETING_DELETION_SIZE 64u
/* Local deletion and remote cancellation are independent. A committed remote
 * meeting is retained and never treated as a remote deletion. */
typedef enum {MEETING_DELETE_REMOTE_PENDING=0,MEETING_DELETE_REMOTE_CANCELLED=1,
              MEETING_DELETE_REMOTE_COMMITTED=2} meeting_delete_remote;
typedef struct {
    uint64_t generation;
    uint32_t file_id;
    uint8_t meeting_id[16],operation_id[16];
    bool local_done;
    /* Old on-device audio without a recoverable meeting identity. The
     * remote state is unknown and no remote cancellation can be attempted. */
    bool legacy_local_only;
    /* A verified zero-frame interrupted capture with its own identity. */
    bool empty_local_only;
    meeting_delete_remote remote;
} meeting_deletion;
typedef struct {
    void *ctx;
    /* 0 present, 1 absent, -1 I/O error. */
    int (*read)(void *ctx,unsigned slot,uint8_t out[MEETING_DELETION_SIZE]);
    bool (*write_sync)(void *ctx,unsigned slot,const uint8_t data[MEETING_DELETION_SIZE]);
} meeting_deletion_io;
typedef enum {MEETING_DELETION_EMPTY,MEETING_DELETION_OK,MEETING_DELETION_DAMAGED,MEETING_DELETION_IO_ERROR} meeting_deletion_status;
bool meeting_deletion_encode(const meeting_deletion *value,uint8_t out[MEETING_DELETION_SIZE]);
bool meeting_deletion_decode(const uint8_t data[MEETING_DELETION_SIZE],meeting_deletion *out);
meeting_deletion_status meeting_deletion_load(const meeting_deletion_io *io,meeting_deletion *out,unsigned *slot);
/* Only monotonic transitions for the same file, meeting and operation. The
 * caller must stop/serialize uploads separately; commit alone does not erase. */
bool meeting_deletion_commit(const meeting_deletion_io *io,const meeting_deletion *next);
