#include "meeting_pairing_session.h"
#include <limits.h>

void meeting_pairing_session_init(meeting_pairing_session *guard)
{
    atomic_init(&guard->generation, 1);
    atomic_init(&guard->session, 0);
    atomic_init(&guard->active, false);
    atomic_init(&guard->exhausted, false);
}

void meeting_pairing_session_reset(meeting_pairing_session *guard)
{
    atomic_store(&guard->active, false);
    unsigned generation = atomic_load(&guard->generation);
    do {
        if (generation == UINT_MAX) {
            atomic_store(&guard->exhausted, true);
            return;
        }
    } while (!atomic_compare_exchange_weak(&guard->generation, &generation, generation + 1));
}

void meeting_pairing_session_accept(meeting_pairing_session *guard, uint32_t session)
{
    if (atomic_load(&guard->active) && atomic_load(&guard->session) != session)
        meeting_pairing_session_reset(guard);
    if (atomic_load(&guard->exhausted)) return;
    atomic_store(&guard->session, session);
    atomic_store(&guard->active, true);
}

bool meeting_pairing_session_capture(const meeting_pairing_session *guard, uint32_t session,
                                     meeting_pairing_token *token)
{
    if (!guard || !token || atomic_load(&guard->exhausted) || !atomic_load(&guard->active)) return false;
    unsigned generation = atomic_load(&guard->generation);
    if (atomic_load(&guard->session) != session || !atomic_load(&guard->active) ||
        atomic_load(&guard->generation) != generation) return false;
    token->generation = generation;
    token->session = session;
    return true;
}

bool meeting_pairing_session_valid(const meeting_pairing_session *guard, meeting_pairing_token token)
{
    if (!guard || atomic_load(&guard->exhausted) || !atomic_load(&guard->active)) return false;
    return atomic_load(&guard->generation) == token.generation &&
           atomic_load(&guard->session) == token.session && atomic_load(&guard->active) &&
           atomic_load(&guard->generation) == token.generation;
}
