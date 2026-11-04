#pragma once
#include "meeting_time_client.h"
/* Called only while the network supervisor owns a running Wi-Fi station.
 * The cancellation callback also prevents recording from waiting on retry. */
meeting_time_client_result meeting_time_device_sync(meeting_time_challenge *challenge,
 const char *https_origin,const char *http_origin,const meeting_time_anchor *anchor,
 bool (*cancelled)(void *),void *cancel_ctx,const meeting_clock_io *clock_io,meeting_clock *clock);
