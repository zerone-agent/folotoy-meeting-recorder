#include "meeting_provision_apply.h"
#include <stdlib.h>
#include <string.h>

meeting_provision_result meeting_provision_apply(const uint8_t *json,size_t length,
    const uint8_t confirmed_ca_sha256[32],int64_t confirmed_utc,bool user_confirmed,
    meeting_session_valid session_valid,void *session_ctx,bool allow_initial_floor,uint64_t received_uptime_ms,uint64_t uptime_ms,
    const meeting_clock_io *clock_io,meeting_clock *clock,const meeting_config_store_io *store_io,
    meeting_ca_validate validate_ca,void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *out)
{
    if(clock)meeting_clock_reset(clock);
    if(out)meeting_config_clear(out);
    if(!json||!confirmed_ca_sha256||!user_confirmed||!session_valid||!clock_io||!clock||!store_io||
       !validate_ca||!scratch||!out)return MEETING_PROVISION_REJECTED;
    if(!session_valid(session_ctx))return MEETING_PROVISION_SESSION_LOST;
    if(!meeting_config_parse(json,length,validate_ca,ca_ctx,out)||out->utc!=confirmed_utc||
       memcmp(out->ca_sha256,confirmed_ca_sha256,32))goto rejected;
    if(!session_valid(session_ctx))goto lost;
    /* The displayed timestamp refers to receipt of the configuration. Account
     * for time spent reviewing it, but reject a reset or stale session. */
    if(uptime_ms<received_uptime_ms||uptime_ms-received_uptime_ms>120000u)goto rejected;
    meeting_config *temporary=calloc(1,sizeof(*temporary));
    if(!temporary){meeting_config_clear(out);return MEETING_PROVISION_STORE_FAILED;}
    bool compatible=meeting_config_store_compatible(store_io,validate_ca,ca_ctx,scratch,out,temporary);
    if(!compatible){meeting_config_clear(temporary);free(temporary);meeting_config_clear(out);return MEETING_PROVISION_STORE_FAILED;}
    if(!session_valid(session_ctx)){meeting_config_clear(temporary);free(temporary);goto lost;}
    int64_t adjusted_utc=out->utc+(int64_t)((uptime_ms-received_uptime_ms)/1000u);
    if(!meeting_clock_bootstrap(clock,clock_io,true,true,allow_initial_floor,adjusted_utc,uptime_ms)){
        meeting_config_clear(temporary);free(temporary);meeting_config_clear(out);return MEETING_PROVISION_CLOCK_FAILED;
    }
    if(!session_valid(session_ctx)){meeting_config_clear(temporary);free(temporary);goto lost;}
    bool committed=meeting_config_store_commit_preparsed(store_io,json,length,validate_ca,ca_ctx,scratch,out,temporary);
    meeting_config_clear(temporary);free(temporary);
    if(!committed){meeting_clock_reset(clock);meeting_config_clear(out);return MEETING_PROVISION_STORE_FAILED;}
    if(!session_valid(session_ctx))goto lost;
    return MEETING_PROVISION_APPLIED;
lost:
    meeting_clock_reset(clock);meeting_config_clear(out);return MEETING_PROVISION_SESSION_LOST;
rejected:
    meeting_clock_reset(clock);meeting_config_clear(out);return MEETING_PROVISION_REJECTED;
}
