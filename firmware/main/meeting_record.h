#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* Persist via an atomic, verified backend before creating or closing audio.
 * A termination intent never proves that the recording completed. */
typedef struct {
    uint32_t file_id;
    uint8_t meeting_id[16];
    uint8_t end_intent; /* 0 none, 1 user, 2 limit, 3 storage, 4 incomplete */
    bool recorded_at_present;
    int64_t recorded_at;
    uint8_t time_trust; /* 0 uncalibrated, 2 trusted */
} meeting_record;
#define MEETING_RECORD_LEGACY_SIZE 32u
#define MEETING_RECORD_SIZE 48u
bool meeting_record_encode(const meeting_record *record, uint8_t out[MEETING_RECORD_SIZE]);
bool meeting_record_decode(const uint8_t *data, size_t length, meeting_record *record);
