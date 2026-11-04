#pragma once
#include "meeting_config.h"
#include "meeting_credential_reset.h"
/* The event ID must be fresh and persisted by the caller through reset_begin.
 * Version 1 configurations lack a device ID and cannot be reset this way. */
bool meeting_reset_identity_from_config(const meeting_config *config,const char *event_id,meeting_reset_record *out);
