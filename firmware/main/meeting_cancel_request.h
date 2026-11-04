#pragma once
#include "meeting_upload_protocol.h"
#include "meeting_receipt_json.h"
/* 0 unknown/retry, 1 cancelled, 2 committed. The transport is authenticated;
 * committed requires the exact retained manifest. This makes no local changes. */
int meeting_cancel_request(const uint8_t meeting_id[16],const meeting_manifest *expected,const meeting_upload_transport *transport);
