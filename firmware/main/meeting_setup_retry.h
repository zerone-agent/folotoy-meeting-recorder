#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Retry only when the caller has verified the device is idle. Elapsed time is
 * monotonic uptime; a backwards clock resets the waiting period safely. */
typedef struct { bool attempted; int64_t last_ms; } meeting_setup_retry;
bool meeting_setup_retry_due(meeting_setup_retry *retry, int64_t now_ms, bool eligible);
