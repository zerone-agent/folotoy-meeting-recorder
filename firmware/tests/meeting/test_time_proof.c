#include "meeting_time_proof.h"
#include <assert.h>
#include <string.h>
typedef struct {int floor_status,writes,sets,verified;bool commit_ok;int64_t floor;} fixture;
static int read_floor(void *ctx,int64_t *floor){fixture *f=ctx;*floor=f->floor;return f->floor_status;}
static bool commit(void *ctx,int64_t floor){fixture *f=ctx;f->writes++;if(!f->commit_ok)return false;f->floor=floor;return true;}
static bool set_time(void *ctx,int64_t epoch){fixture *f=ctx;(void)epoch;f->sets++;return true;}
static bool verify(void *ctx,const uint8_t *m,size_t n,const uint8_t *sig,size_t sn){fixture *f=ctx;(void)m;(void)sig;assert(n==76&&sn==8);return f->verified;}
static void message(uint8_t *m,const uint8_t *nonce,const uint8_t *origin,uint64_t t){memcpy(m,"MTP1",4);memcpy(m+4,nonce,32);memcpy(m+36,origin,32);for(unsigned i=0;i<8;i++)m[75-i]=(uint8_t)(t>>(8*i));}
int main(void){
 fixture f={.floor=1800000000,.commit_ok=true,.verified=true};meeting_clock_io io={&f,read_floor,commit,set_time};meeting_clock clock={0};meeting_time_challenge c={0};
 uint8_t nonce[32]={1},origin[32]={2},m[76],sig[8]={0};message(m,nonce,origin,1800000001);
 assert(meeting_time_challenge_begin(&c,nonce,origin,100));assert(!meeting_time_challenge_begin(&c,nonce,origin,101));
 m[4]^=1;assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,200,verify,&f,&io,&clock)&&!f.writes);
 m[4]^=1;assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,200,verify,&f,&io,&clock));
 assert(!meeting_time_challenge_begin(&c,nonce,origin,201));nonce[0]++;message(m,nonce,origin,1800000001);
 assert(meeting_time_challenge_begin(&c,nonce,origin,250));m[36]^=1;
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,251,verify,&f,&io,&clock)&&!f.writes);m[36]^=1;
 nonce[0]++;message(m,nonce,origin,1800000001);assert(meeting_time_challenge_begin(&c,nonce,origin,260));
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,259,verify,&f,&io,&clock)&&!f.writes);
 nonce[0]++;message(m,nonce,origin,1800000001);
 assert(meeting_time_challenge_begin(&c,nonce,origin,300));f.verified=false;assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,400,verify,&f,&io,&clock)&&!f.writes);
 nonce[0]++;message(m,nonce,origin,1800000001);assert(meeting_time_challenge_begin(&c,nonce,origin,500));f.verified=true;
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,15501,verify,&f,&io,&clock)&&!f.writes);
 nonce[0]++;message(m,nonce,origin,1800000001);assert(meeting_time_challenge_begin(&c,nonce,origin,600));f.floor_status=1;
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,700,verify,&f,&io,&clock)&&!f.writes);
 nonce[0]++;message(m,nonce,origin,1799999999);assert(meeting_time_challenge_begin(&c,nonce,origin,800));f.floor_status=0;
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,900,verify,&f,&io,&clock)&&!f.writes);
 nonce[0]++;message(m,nonce,origin,1800000001);assert(meeting_time_challenge_begin(&c,nonce,origin,1000));f.commit_ok=false;
 assert(!meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,1100,verify,&f,&io,&clock)&&!f.sets&&!clock.trusted);
 nonce[0]++;message(m,nonce,origin,1800000001);assert(meeting_time_challenge_begin(&c,nonce,origin,1200));f.commit_ok=true;
 assert(meeting_time_proof_apply(&c,m,sizeof m,sig,sizeof sig,1300,verify,&f,&io,&clock)&&f.floor==1800000001&&f.sets==1&&clock.trusted);
 return 0;
}
