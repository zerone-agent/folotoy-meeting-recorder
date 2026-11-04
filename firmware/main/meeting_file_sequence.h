#pragma once
#include <stdbool.h>
#include <stdint.h>

/* The caller serializes allocation. read returns 0 for an existing value,
 * 1 for a missing value and -1 on error. write_sync must commit durably. */
typedef struct {
    void *ctx;
    int (*read)(void *ctx,uint32_t *value);
    bool (*write_sync)(void *ctx,uint32_t value);
} meeting_file_sequence_io;
/* Does not publish an ID until it has been committed and read back. A failed
 * or ambiguous commit may consume an ID, but must never create a recording. */
bool meeting_file_sequence_next(const meeting_file_sequence_io *io,uint32_t baseline,
                                uint32_t wall_id,uint32_t *out);
