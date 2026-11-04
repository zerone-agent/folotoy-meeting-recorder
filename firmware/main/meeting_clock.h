#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Only an authenticated, replay-protected provisioning session may call
 * bootstrap with both authorizations set. Reboot starts untrusted, even if a
 * floor exists. The storage callback must atomically commit and verify. */
typedef struct {
    void *ctx;
    int (*read_floor)(void *ctx, int64_t *floor); /* 0 present, 1 absent, -1 error */
    bool (*commit_floor)(void *ctx, int64_t floor);
    bool (*set_wall_time)(void *ctx, int64_t epoch_seconds);
} meeting_clock_io;
typedef struct {
    bool trusted;
    int64_t epoch_at_sync;
    uint64_t uptime_at_sync_ms;
    int64_t persisted_floor;
} meeting_clock;
void meeting_clock_reset(meeting_clock *clock);
bool meeting_clock_bootstrap(meeting_clock *clock,const meeting_clock_io *io,bool authenticated,
                             bool user_confirmed,bool allow_initial_floor,int64_t epoch,uint64_t uptime_ms);
bool meeting_clock_valid(meeting_clock *clock,int64_t wall_epoch,uint64_t uptime_ms);
bool meeting_clock_checkpoint(meeting_clock *clock,const meeting_clock_io *io,int64_t wall_epoch,uint64_t uptime_ms);
