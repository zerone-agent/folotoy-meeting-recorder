#pragma once
#include "meeting_time_anchor.h"
#include "meeting_time_proof.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
 void *ctx;
 bool (*random)(void *ctx,uint8_t nonce[32]); /* cryptographic hardware RNG */
 bool (*uptime_ms)(void *ctx,uint64_t *out); /* monotonic, current boot */
 bool (*cancelled)(void *ctx);
 bool (*fetch)(void *ctx,const uint8_t nonce[32],char *response,size_t capacity,size_t *length,int *status);
} meeting_time_client_io;
typedef enum {MEETING_TIME_CLIENT_OK,MEETING_TIME_CLIENT_PAUSED,MEETING_TIME_CLIENT_FAILED} meeting_time_client_result;
/* Caller first loads the anchor using the committed origin, owns the Wi-Fi
 * radio, serializes this operation with recording/reset, and keeps challenge
 * state for the whole boot. No credential is passed to fetch. */
meeting_time_client_result meeting_time_client_run(meeting_time_challenge *challenge,const char *https_origin,
 const meeting_time_anchor *anchor,const meeting_time_client_io *client,const meeting_clock_io *clock_io,meeting_clock *clock);
