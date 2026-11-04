#pragma once
#include "meeting_time_anchor.h"
/* Maintenance-only: the connected agent authenticates the server using
 * HTTPS and flashes a device-specific image containing its public key. */
meeting_time_anchor_result meeting_agent_anchor_install(meeting_time_anchor_session *session,
 const meeting_time_anchor_io *io,const uint8_t configured_origin[32],const char *expected_origin_hex,
 const char *public_key_hex,bool device_allowed,meeting_time_key_check check,void *key_ctx);
bool meeting_agent_anchor_matches(const meeting_time_anchor *anchor,const uint8_t configured_origin[32],
 const char *expected_origin_hex,const char *public_key_hex,bool device_allowed);
