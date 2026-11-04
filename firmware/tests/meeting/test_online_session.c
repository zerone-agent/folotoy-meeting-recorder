#include "meeting_online_session.h"
#include <assert.h>
static int ip=1,recording,clock_ok=1,calls;
static bool has_ip(void *p){(void)p;return ip;}
static bool cancel(void *p){(void)p;return recording;}
static bool time_ok(void *p){(void)p;return clock_ok;}
meeting_upload_result meeting_upload_session(FILE *f,const meeting_snapshot *s,bool h,int64_t t,uint8_t trust,meeting_https_config *c,meeting_manifest *confirmed){(void)confirmed;(void)f;(void)s;(void)h;(void)t;(void)trust;calls++;assert(c->time_valid(c->cancel_ctx)==clock_ok);ip=0;assert(c->is_cancelled(c->cancel_ctx));return MEETING_UPLOAD_PAUSED;}
int main(void){meeting_online_config cfg={.https={.is_cancelled=cancel,.time_valid=time_ok},.has_ip=has_ip};meeting_snapshot snapshot={0};FILE *file=tmpfile();assert(file);assert(meeting_online_session(file,&snapshot,false,0,0,&cfg,NULL)==MEETING_UPLOAD_PAUSED&&calls==1);assert(meeting_online_session(file,&snapshot,false,0,0,&cfg,NULL)==MEETING_UPLOAD_PAUSED&&calls==1);ip=1;recording=1;assert(meeting_online_session(file,&snapshot,false,0,0,&cfg,NULL)==MEETING_UPLOAD_PAUSED&&calls==1);fclose(file);return 0;}
