#pragma once
#include "meeting_clock.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Candidate protocol MTP1: magic(4), nonce(32), SHA256(origin)(32),
 * unsigned big-endian UTC seconds(8), signed by a provisioned P-256 key.
 * The transport may be untrusted. The caller must zero-initialize this state,
 * generate each nonce using a cryptographic RNG, serialize requests and
 * retain this state for the boot. */
#define MEETING_TIME_NONCE_SIZE 32u
#define MEETING_TIME_PROOF_SIZE 76u
#define MEETING_TIME_PROOF_MAX_SIGNATURE 72u
#define MEETING_TIME_PROOF_MAX_AGE_MS 15000u
typedef struct {
    uint8_t nonce[MEETING_TIME_NONCE_SIZE];
    uint8_t origin_sha256[32];
    uint64_t started_ms;
    bool active, have_nonce;
} meeting_time_challenge;
typedef bool (*meeting_time_verify_fn)(void *ctx,const uint8_t *message,size_t message_len,
                                       const uint8_t *signature,size_t signature_len);
/* Refuses the immediately previous nonce; freshness across all requests and
 * reboots depends on the caller's cryptographic RNG. */
bool meeting_time_challenge_begin(meeting_time_challenge *challenge,const uint8_t nonce[32],
                                  const uint8_t origin_sha256[32],uint64_t uptime_ms);
/* Abandon without allowing the most recent nonce to be reused. */
void meeting_time_challenge_cancel(meeting_time_challenge *challenge);
/* One-shot: every attempted response consumes the challenge, including an
 * invalid response. Never initializes a missing floor or bypasses its commit. */
bool meeting_time_proof_apply(meeting_time_challenge *challenge,const uint8_t *message,size_t message_len,
                              const uint8_t *signature,size_t signature_len,uint64_t uptime_ms,
                              meeting_time_verify_fn verify,void *verify_ctx,
                              const meeting_clock_io *clock_io,meeting_clock *clock);
