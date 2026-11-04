#include "meeting_time_anchor.h"
#include <assert.h>
#include <string.h>
typedef struct {uint8_t staged[MEETING_TIME_ANCHOR_SIZE],durable[MEETING_TIME_ANCHOR_SIZE];bool visible,persisted,fail,commit_then_fail;int writes,checks,deny_at;} fixture;
static int read_data(void *ctx,uint8_t *out){fixture *f=ctx;if(!f->visible)return 1;memcpy(out,f->staged,sizeof f->staged);return 0;}
static bool write_data(void *ctx,const uint8_t *data){fixture *f=ctx;f->writes++;memcpy(f->staged,data,sizeof f->staged);f->visible=true;if(!f->fail||f->commit_then_fail){memcpy(f->durable,data,sizeof f->durable);f->persisted=true;}return !f->fail;}
static bool auth(void *ctx,const meeting_time_anchor *a){fixture *f=ctx;assert(a);f->checks++;return f->checks!=f->deny_at;}
static bool key_ok(void *ctx,const meeting_time_public_key *key){(void)ctx;return key->public_key[0]==4&&key->public_key[1]==7;}
static void reboot(fixture *f,meeting_time_anchor_session *s){f->visible=f->persisted;if(f->persisted)memcpy(f->staged,f->durable,sizeof f->staged);*s=(meeting_time_anchor_session){0};}
int main(void){
 fixture f={0};meeting_time_anchor_io io={&f,read_data,write_data};meeting_time_anchor_session session={0};
 meeting_time_anchor a={0},out;a.origin_sha256[0]=3;a.key.public_key[0]=4;a.key.public_key[1]=7;
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ABSENT);
 f.deny_at=1;assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_DENIED&&!f.writes);
 f.deny_at=0;f.checks=0;f.fail=true;
 assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_ERROR&&session.uncertain&&f.visible);
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ERROR);
 assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_ERROR&&session.uncertain);
 f.fail=false;assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_OK&&!session.uncertain);
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_OK&&!memcmp(&a,&out,sizeof a));
 meeting_time_anchor other=a;other.origin_sha256[1]=9;int writes=f.writes;
 assert(meeting_time_anchor_install(&session,&io,&other,auth,&f,key_ok,NULL)==MEETING_ANCHOR_CONFLICT&&f.writes==writes);
 assert(meeting_time_anchor_load(&session,&io,other.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_CONFLICT);
 f.staged[50]^=1;assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ERROR);
 assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_ERROR&&f.writes==writes);
 reboot(&f,&session);assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_OK);
 /* Losing authorization after commit cannot enable the record this boot. */
 f.checks=0;f.deny_at=3;assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_DENIED&&session.uncertain);
 assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_ERROR);
 f.deny_at=0;assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_OK);
 /* An acknowledged failure may still have committed; reboot reads only the
  * durable copy, and matching authenticated retry remains idempotent. */
 f.fail=true;f.commit_then_fail=true;assert(meeting_time_anchor_install(&session,&io,&a,auth,&f,key_ok,NULL)==MEETING_ANCHOR_ERROR);
 reboot(&f,&session);assert(meeting_time_anchor_load(&session,&io,a.origin_sha256,key_ok,NULL,&out)==MEETING_ANCHOR_OK);
 return 0;
}
