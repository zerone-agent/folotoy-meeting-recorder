#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Single serialized caller. A cancelled lwIP lookup may complete later; it
 * retains no caller-owned pointers and a new lookup waits for its cleanup. */
bool meeting_dns_resolve(const char *hostname,bool (*cancelled)(void *),void *ctx,
                         int64_t deadline_us,char ipv4[16]);
