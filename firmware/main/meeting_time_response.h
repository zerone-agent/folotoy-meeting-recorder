#pragma once
#include "meeting_time_proof.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {uint8_t message[MEETING_TIME_PROOF_SIZE],signature[MEETING_TIME_PROOF_MAX_SIGNATURE];size_t signature_len;} meeting_time_response;
/* expected_key_id is SHA-256 of the locally pinned 65-byte public point;
 * never derive it from the untrusted response. This parser does not verify a
 * signature or authorize time: use meeting_time_proof_apply afterwards. */
bool meeting_time_response_parse(const char *json,size_t length,int http_status,const uint8_t expected_key_id[32],meeting_time_response *out);
