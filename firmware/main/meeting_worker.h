#pragma once
#include "meeting_online_session.h"
#include "sc_proto.h"

/* Caller supplies scratch space and a stable, authenticated configuration.
 * Each pass examines the current catalog oldest-first. A paused transfer ends
 * the pass so recording can take ownership. No audio is deleted here. */
typedef struct { unsigned attempted, confirmed, failed; bool paused; } meeting_worker_result;
meeting_worker_result meeting_worker_pass(const meeting_online_config *config, sc_file_entry_t *scratch, size_t capacity);
