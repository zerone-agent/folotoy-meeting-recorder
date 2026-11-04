#pragma once
#include "meeting_time_signature.h"
#include <stdbool.h>
#include <stdint.h>
#define MEETING_TIME_ANCHOR_SIZE 108u

typedef struct {uint8_t origin_sha256[32];meeting_time_public_key key;} meeting_time_anchor;
typedef struct {void *ctx;int (*read)(void *ctx,uint8_t out[MEETING_TIME_ANCHOR_SIZE]);bool (*write_sync)(void *ctx,const uint8_t data[MEETING_TIME_ANCHOR_SIZE]);} meeting_time_anchor_io;
typedef bool (*meeting_time_key_check)(void *ctx,const meeting_time_public_key *key);
typedef bool (*meeting_time_anchor_authorized)(void *ctx,const meeting_time_anchor *anchor);
/* One state per boot and storage instance. Do not reset it after a failed
 * write: staged NVS data can remain visible until a successful recommit. */
typedef struct {bool uncertain;} meeting_time_anchor_session;
typedef enum {MEETING_ANCHOR_OK,MEETING_ANCHOR_ABSENT,MEETING_ANCHOR_DENIED,MEETING_ANCHOR_CONFLICT,MEETING_ANCHOR_ERROR} meeting_time_anchor_result;
bool meeting_time_anchor_encode(const meeting_time_anchor *value,uint8_t out[MEETING_TIME_ANCHOR_SIZE]);
bool meeting_time_anchor_decode(const uint8_t data[MEETING_TIME_ANCHOR_SIZE],meeting_time_anchor *out);
/* Serialize against configuration/reset. authorization must independently
 * authenticate the administrator and current secure transport, and bind the
 * exact origin and public key. This function never replaces an existing key. */
meeting_time_anchor_result meeting_time_anchor_install(meeting_time_anchor_session *session,const meeting_time_anchor_io *io,
    const meeting_time_anchor *candidate,meeting_time_anchor_authorized authorization,void *auth_ctx,
    meeting_time_key_check check_key,void *key_ctx);
/* Use the origin derived from the committed configuration, not from a network
 * response. No storage writes or first-time initialization occur here. */
meeting_time_anchor_result meeting_time_anchor_load(const meeting_time_anchor_session *session,const meeting_time_anchor_io *io,
    const uint8_t expected_origin_sha256[32],meeting_time_key_check check_key,void *key_ctx,meeting_time_anchor *out);
