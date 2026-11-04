#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Only constructed after strict JSON parsing, including mandatory field and
 * duplicate-key checks. A match is not a persistent deletion authorization. */
typedef struct {
    char id[37];
    uint32_t count;
    uint64_t bytes;
    uint8_t digest[32];
    uint8_t reason; /* 0 user, 1 limit, 2 storage, 3 interrupted */
    bool recorded_at_present;
    int64_t recorded_at;
    uint8_t time_trust; /* 0 uncalibrated, 1 unverified, 2 trusted */
} meeting_manifest;
bool meeting_receipt_matches(const meeting_manifest *expected, const meeting_manifest *received, bool committed);
