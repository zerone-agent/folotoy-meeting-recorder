#pragma once
#include "meeting_clock.h"
#include "meeting_config.h"
#include "meeting_time_anchor.h"
#include "esp_err.h"
#include <stdbool.h>
typedef enum { MEETING_NETWORK_OFF, MEETING_NETWORK_CONNECTING, MEETING_NETWORK_CLOCK_REQUIRED,
               MEETING_NETWORK_SYNCING, MEETING_NETWORK_IDLE, MEETING_NETWORK_RETRY, MEETING_NETWORK_STOPPING } meeting_network_state;

/* Task-only, single invocation serialized with the core's legacy Wi-Fi
 * switching (including its pending state), after the coordinator has
 * authenticated, confirmed and durably committed config and trusted time.
 * clock_io and its context must remain valid until the device restarts.
 * Reset/reprovision is not supported until revocation is implemented. */
esp_err_t meeting_network_activate(const meeting_config *config,const meeting_clock *clock,const meeting_clock_io *clock_io);
/* Restore a previously authenticated configuration and already committed
 * anchor. HTTP is only used for signed time, never for credentials. */
esp_err_t meeting_network_activate_restored(const meeting_config *config,const meeting_time_anchor *anchor,
                                           const char *http_origin,const meeting_clock_io *clock_io);
bool meeting_network_enabled(void);
/* A timestamp is trusted only while this boot's validated clock remains valid. */
bool meeting_network_trusted_time(int64_t *epoch_seconds);
typedef enum { MEETING_CLOCK_OFF, MEETING_CLOCK_CHECKING, MEETING_CLOCK_TRUSTED } meeting_clock_view;
/* Nonblocking UI snapshot; UTC offset is set only for a trusted clock. */
meeting_clock_view meeting_network_clock_view(int16_t *timezone_minutes);
/* Core/task context only. Cancel in-flight requests, stop Wi-Fi, wipe the
 * in-memory configuration and clock, then report completion. The caller
 * must separately serialize this operation with recording and provisioning. */
bool meeting_network_stop_request(void);
bool meeting_network_stop_complete(void);
/* Wake the upload supervisor after a recording has been durably saved. */
void meeting_network_record_saved(void);
/* Recording takes precedence: request an asynchronous Wi-Fi shutdown and
 * wait for readiness before allocating audio resources. Release on every
 * completion, cancellation, failure, or timeout. */
void meeting_network_record_request(void);
bool meeting_network_record_ready(void);
void meeting_network_record_release(void);
meeting_network_state meeting_network_get_state(void);
