#pragma once
#include <stdbool.h>
#include <stdint.h>
#define MEETING_SEAL_SIZE 48u
/* Proof is written only after the source and index were synced and closed.
 * It is not a content hash: recovery must independently scan the source and
 * match both its complete frame count and exact length before trusting it. */
typedef struct {uint32_t file_id;uint8_t meeting_id[16];uint32_t frames;uint64_t bytes;uint8_t reason;} meeting_seal;
bool meeting_seal_encode(const meeting_seal *seal,uint8_t out[MEETING_SEAL_SIZE]);
bool meeting_seal_decode(const uint8_t data[MEETING_SEAL_SIZE],meeting_seal *seal);
