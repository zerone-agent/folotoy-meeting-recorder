#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEETING_RETIRED_SIZE 32u
typedef struct {uint32_t file_id;uint8_t meeting_id[16];bool complete;} meeting_retired;
typedef enum {MEETING_RETIRE_SNAPSHOT_0,MEETING_RETIRE_SNAPSHOT_1,MEETING_RETIRE_RECEIPT,
              MEETING_RETIRE_RECORD,MEETING_RETIRE_INTENT,MEETING_RETIRE_SEAL,MEETING_RETIRE_PARTS} meeting_retire_part;
typedef struct {
    void *ctx;
    bool (*cancelled)(void *ctx);
    /* 0 present, 1 absent, -1 error. */
    int (*read_marker)(void *ctx,uint8_t out[MEETING_RETIRED_SIZE]);
    bool (*write_marker_sync)(void *ctx,const uint8_t marker[MEETING_RETIRED_SIZE]);
    /* Must establish files were absent after a fresh mount and the saved
     * receipt, cleanup intent, record and snapshot are consistent. */
    bool (*verify_completed)(void *ctx,const meeting_retired *id);
    bool (*files_absent)(void *ctx);
    /* Erase must be idempotent, committed and verified. */
    bool (*erase_part_sync)(void *ctx,meeting_retire_part part);
} meeting_retirement_io;
typedef enum {MEETING_RETIRE_DONE,MEETING_RETIRE_PAUSED,MEETING_RETIRE_ERROR} meeting_retirement_result;
bool meeting_retired_encode(const meeting_retired *value,uint8_t out[MEETING_RETIRED_SIZE]);
bool meeting_retired_decode(const uint8_t data[MEETING_RETIRED_SIZE],meeting_retired *out);
/* A newly created marker is only allowed after a fresh mount. Existing markers
 * can finish partial metadata cleanup without the erased receipt/snapshot. */
meeting_retirement_result meeting_retirement_run(const meeting_retired *identity,bool fresh_mount,
                                                  const meeting_retirement_io *io);
