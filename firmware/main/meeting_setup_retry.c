#include "meeting_setup_retry.h"
#define RETRY_INTERVAL_MS 30000LL
bool meeting_setup_retry_due(meeting_setup_retry *retry, int64_t now_ms, bool eligible)
{
    if(!retry || now_ms<0 || !eligible)return false;
    if(retry->attempted && now_ms<retry->last_ms){retry->last_ms=now_ms;return false;}
    if(retry->attempted && now_ms-retry->last_ms<RETRY_INTERVAL_MS)return false;
    retry->attempted=true;retry->last_ms=now_ms;return true;
}
