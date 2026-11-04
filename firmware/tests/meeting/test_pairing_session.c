#include "meeting_pairing_session.h"
#include <assert.h>
#include <limits.h>

int main(void)
{
    meeting_pairing_session guard;
    meeting_pairing_token first, second;
    meeting_pairing_session_init(&guard);
    assert(!meeting_pairing_session_capture(&guard, 7, &first));
    meeting_pairing_session_accept(&guard, 7);
    assert(meeting_pairing_session_capture(&guard, 7, &first));
    assert(meeting_pairing_session_valid(&guard, first));
    meeting_pairing_session_reset(&guard);
    assert(!meeting_pairing_session_valid(&guard, first));
    meeting_pairing_session_accept(&guard, 7);
    assert(meeting_pairing_session_capture(&guard, 7, &second));
    assert(!meeting_pairing_session_valid(&guard, first));
    assert(meeting_pairing_session_valid(&guard, second));
    meeting_pairing_session_accept(&guard, 8);
    meeting_pairing_session_accept(&guard, 7);
    assert(!meeting_pairing_session_valid(&guard, second));
    atomic_store(&guard.generation, UINT_MAX);
    meeting_pairing_session_reset(&guard);
    meeting_pairing_session_accept(&guard, 7);
    assert(!meeting_pairing_session_capture(&guard, 7, &second));
    return 0;
}
