#pragma once
#include "meeting_config_fragments.h"
/* Security 2 plaintext frames: 01 + u16le total, 02 + u16le offset + data,
 * 03 to seal. Return false on malformed, cross-session, or replayed frames.
 * The caller must reset on every disconnect before accepting another peer. */
bool meeting_config_protocol_request(meeting_config_fragments *state,uint32_t session,
                                     const uint8_t *request,size_t length);
