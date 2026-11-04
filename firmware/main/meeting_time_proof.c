#include "meeting_time_proof.h"
#include <string.h>
#define MAX_EPOCH 4102444800ULL
static bool nonzero(const uint8_t *p,size_t n){uint8_t v=0;for(size_t i=0;i<n;i++)v|=p[i];return v!=0;}
bool meeting_time_challenge_begin(meeting_time_challenge *c,const uint8_t nonce[32],const uint8_t origin[32],uint64_t now)
{
 if(!c||!nonce||!origin||c->active||!nonzero(nonce,32)||!nonzero(origin,32)||
    (c->have_nonce&&!memcmp(c->nonce,nonce,32)))return false;
 memcpy(c->nonce,nonce,32);memcpy(c->origin_sha256,origin,32);c->started_ms=now;c->active=true;c->have_nonce=true;return true;
}
void meeting_time_challenge_cancel(meeting_time_challenge *c){if(c)c->active=false;}
bool meeting_time_proof_apply(meeting_time_challenge *c,const uint8_t *message,size_t length,
                              const uint8_t *signature,size_t siglen,uint64_t now,
                              meeting_time_verify_fn verify,void *verify_ctx,const meeting_clock_io *io,meeting_clock *clock)
{
 if(!c||!c->active)return false;
 c->active=false;
 if(!message||length!=MEETING_TIME_PROOF_SIZE||!signature||siglen<8||siglen>MEETING_TIME_PROOF_MAX_SIGNATURE||
    !verify||!io||!clock||now<c->started_ms||now-c->started_ms>MEETING_TIME_PROOF_MAX_AGE_MS||
    memcmp(message,"MTP1",4)||memcmp(message+4,c->nonce,32)||memcmp(message+36,c->origin_sha256,32))return false;
 uint64_t epoch=0;for(unsigned i=0;i<8;i++)epoch=(epoch<<8)|message[68+i];
 if(epoch>MAX_EPOCH||!verify(verify_ctx,message,length,signature,siglen))return false;
 /* Signature authenticates the time but does not grant first commissioning. */
 return meeting_clock_bootstrap(clock,io,true,true,false,(int64_t)epoch,now);
}
