#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* scr1-projection-v1: apply a stable projection to bytes read from the
 * immutable source. length and offset must lie entirely inside valid_end. */
bool meeting_scr1_project(uint64_t offset, uint8_t *buffer, size_t length,
                          uint64_t valid_end, uint32_t frames);

/* read_at returns the bytes read, or -1 on an I/O error. A short record at
 * EOF is a recoverable incomplete tail; malformed complete frames fail. */
typedef int (*meeting_scr1_read_at)(void *ctx, uint64_t offset, uint8_t *out, size_t size);
typedef struct { uint64_t valid_end; uint32_t frames; bool incomplete_tail; } meeting_scr1_scan_result;
bool meeting_scr1_scan(meeting_scr1_read_at read_at, void *ctx, uint64_t file_size,
                        meeting_scr1_scan_result *out);
