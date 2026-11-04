#include "meeting_agent_anchor.h"
#include <assert.h>
#include <string.h>
typedef struct {bool present;uint8_t record[MEETING_TIME_ANCHOR_SIZE];} store;
static int read_record(void *ctx,uint8_t *out){store *s=ctx;if(!s->present)return 1;memcpy(out,s->record,sizeof s->record);return 0;}
static bool write_record(void *ctx,const uint8_t *data){store *s=ctx;memcpy(s->record,data,sizeof s->record);s->present=true;return true;}
static bool check(void *ctx,const meeting_time_public_key *key){(void)ctx;return key->public_key[0]==4;}
int main(void){
 store saved={0};meeting_time_anchor_io io={&saved,read_record,write_record};meeting_time_anchor_session session={0};
 uint8_t origin[32];memset(origin,0x11,sizeof origin);char hex[65],key[131];memset(hex,'1',64);hex[64]=0;memset(key,'2',130);key[0]='0';key[1]='4';key[130]=0;
 assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,false,check,NULL)==MEETING_ANCHOR_DENIED&&!saved.present);
 origin[0]++;assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,true,check,NULL)==MEETING_ANCHOR_DENIED&&!saved.present);origin[0]--;
 key[2]='Z';assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,true,check,NULL)==MEETING_ANCHOR_DENIED&&!saved.present);key[2]='2';
 assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,true,check,NULL)==MEETING_ANCHOR_OK&&saved.present);
 meeting_time_anchor loaded;assert(meeting_time_anchor_load(&session,&io,origin,check,NULL,&loaded)==MEETING_ANCHOR_OK);
 assert(meeting_agent_anchor_matches(&loaded,origin,hex,key,true));
 assert(!meeting_agent_anchor_matches(&loaded,origin,hex,key,false));
 assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,true,check,NULL)==MEETING_ANCHOR_OK);
 key[2]='3';assert(meeting_agent_anchor_install(&session,&io,origin,hex,key,true,check,NULL)==MEETING_ANCHOR_CONFLICT);
 assert(!meeting_agent_anchor_matches(&loaded,origin,hex,key,true));
 return 0;
}
