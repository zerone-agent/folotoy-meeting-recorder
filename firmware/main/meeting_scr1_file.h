#pragma once
#include "meeting_scr1.h"
#include <stdio.h>
typedef bool (*meeting_cancel_fn)(void *ctx);
/* Inspect without writing; caller retains an open handle and must prevent concurrent writers. */
bool meeting_scr1_scan_file(FILE *file, meeting_scr1_scan_result *out);
bool meeting_scr1_scan_file_cancel(FILE *file, meeting_scr1_scan_result *out, meeting_cancel_fn cancelled, void *ctx);
/* Read projected bytes at absolute offset; short reads and bounds violations fail. */
bool meeting_scr1_read_projected(FILE *file, const meeting_scr1_scan_result *scan,
                                 uint64_t offset, uint8_t *out, size_t length);
