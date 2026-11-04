#include "meeting_online_session.h"

typedef struct {const meeting_online_config *config;} session_context;
static bool cancelled(void *ctx)
{
 session_context *s=ctx;
 return !s->config->has_ip(s->config->network_ctx) || s->config->https.is_cancelled(s->config->https.cancel_ctx);
}
static bool time_valid(void *ctx)
{
 session_context *s=ctx;
 return s->config->https.time_valid(s->config->https.cancel_ctx);
}
meeting_upload_result meeting_online_session(FILE *source,const meeting_snapshot *snapshot,
                                              bool has_time,int64_t recorded_at,uint8_t time_trust,
                                              const meeting_online_config *config, meeting_manifest *confirmed)
{
 if(!config||!config->has_ip||!config->https.is_cancelled||!config->https.time_valid)return MEETING_UPLOAD_FAILED;
 session_context context={.config=config};
 if(cancelled(&context))return MEETING_UPLOAD_PAUSED;
 meeting_https_config https=config->https;
 https.cancel_ctx=&context;https.is_cancelled=cancelled;https.time_valid=time_valid;
 return meeting_upload_session(source,snapshot,has_time,recorded_at,time_trust,&https,confirmed);
}
