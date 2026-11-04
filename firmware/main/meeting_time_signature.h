#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* SEC1 uncompressed P-256 point. Provisioning must bind it to the origin. */
typedef struct { uint8_t public_key[65]; } meeting_time_public_key;
bool meeting_time_public_key_valid(void *ctx,const meeting_time_public_key *key);
bool meeting_time_signature_verify(void *ctx,const uint8_t *message,size_t message_len,
                                   const uint8_t *signature,size_t signature_len);
