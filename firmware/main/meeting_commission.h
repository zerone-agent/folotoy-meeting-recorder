#pragma once
#include <stdbool.h>
#include <stdint.h>
/* The MAC scopes a supervised prototype build; it is not an authenticator. */
bool meeting_commission_mac_matches(const char *configured,const uint8_t actual[6]);
bool meeting_commission_allowed(void);
