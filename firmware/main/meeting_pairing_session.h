#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

/* BLE callbacks are serialized by the transport. The stop path may invalidate
 * concurrently; all other tasks may only capture and validate tokens. */
typedef struct {
    atomic_uint generation;
    atomic_uint session;
    atomic_bool active;
    atomic_bool exhausted;
} meeting_pairing_session;
typedef struct { unsigned generation, session; } meeting_pairing_token;

void meeting_pairing_session_init(meeting_pairing_session *guard);
void meeting_pairing_session_reset(meeting_pairing_session *guard);
void meeting_pairing_session_accept(meeting_pairing_session *guard, uint32_t session);
bool meeting_pairing_session_capture(const meeting_pairing_session *guard, uint32_t session,
                                     meeting_pairing_token *token);
bool meeting_pairing_session_valid(const meeting_pairing_session *guard, meeting_pairing_token token);
