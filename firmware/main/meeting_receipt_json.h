#pragma once
#include "meeting_receipt.h"
#include <stddef.h>
/* Bounded response, requires HTTP 200, all fields exactly once, no trailing data.
 * A match never itself permits deletion: persist and reconcile first. */
bool meeting_receipt_json_matches(const char *json, size_t length, int http_status, const meeting_manifest *expected);
bool meeting_receipt_json_integrity_error(const char *json,size_t length,int http_status);
/* Authenticated response to a request for the intended meeting. A committed
 * result additionally requires the saved expected manifest to match exactly. */
typedef enum {MEETING_CANCEL_UNKNOWN,MEETING_CANCEL_CANCELLED,MEETING_CANCEL_COMMITTED} meeting_cancel_response;
meeting_cancel_response meeting_cancel_json_parse(const char *json,size_t length,int http_status,const meeting_manifest *expected);
/* On success returns a nonempty bitmap of original chunks safe to retry.
 * Missing server metadata, ambiguous integrity and oversized responses fail. */
bool meeting_repair_json_parse(const char *json,size_t length,int http_status,const meeting_manifest *expected,
                               uint8_t *bitmap,size_t bitmap_size);
typedef struct { bool present,valid; uint32_t size; uint8_t digest[32]; } meeting_remote_chunk;
bool meeting_chunk_window_json_parse(const char *json,size_t length,int http_status,const char *id,
                                     uint32_t first,meeting_remote_chunk entries[16]);
bool meeting_legacy_chunk_window_json_parse(const char *json,size_t length,int http_status,const char *id,
                                            uint32_t first,meeting_remote_chunk entries[16]);
