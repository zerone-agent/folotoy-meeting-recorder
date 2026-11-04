#include "meeting_time_client.h"
#include "meeting_time_response.h"
#include "mbedtls/sha256.h"
#include <string.h>
typedef struct {const meeting_time_client_io *io;const meeting_time_public_key *key;uint64_t start_ms;} verify_context;
static bool verify_fresh(void *ctx,const uint8_t *message,size_t n,const uint8_t *signature,size_t sn){
 verify_context *v=ctx;uint64_t now;
 if(v->io->cancelled(v->io->ctx)||!meeting_time_signature_verify((void *)v->key,message,n,signature,sn)||
    v->io->cancelled(v->io->ctx)||!v->io->uptime_ms(v->io->ctx,&now)||now<v->start_ms||now-v->start_ms>MEETING_TIME_PROOF_MAX_AGE_MS)return false;
 return true;
}
meeting_time_client_result meeting_time_client_run(meeting_time_challenge *challenge,const char *origin,
 const meeting_time_anchor *anchor,const meeting_time_client_io *client,const meeting_clock_io *clock_io,meeting_clock *clock){
 if(!challenge||!origin||!anchor||!client||!client->random||!client->uptime_ms||!client->cancelled||!client->fetch||!clock_io||!clock||clock->trusted)return MEETING_TIME_CLIENT_FAILED;
 if(client->cancelled(client->ctx))return MEETING_TIME_CLIENT_PAUSED;
 size_t origin_len=0;while(origin_len<=200&&origin[origin_len])origin_len++;
 if(origin_len>200||origin_len<9||strncmp(origin,"https://",8))return MEETING_TIME_CLIENT_FAILED;
 uint8_t origin_hash[32],key_id[32],nonce[32];uint64_t started,received;
 if(mbedtls_sha256((const uint8_t *)origin,origin_len,origin_hash,0)!=0||memcmp(origin_hash,anchor->origin_sha256,32)||
    mbedtls_sha256(anchor->key.public_key,sizeof anchor->key.public_key,key_id,0)!=0||!meeting_time_public_key_valid(NULL,&anchor->key)||
    !client->random(client->ctx,nonce)||!client->uptime_ms(client->ctx,&started)||!meeting_time_challenge_begin(challenge,nonce,origin_hash,started))return MEETING_TIME_CLIENT_FAILED;
 char body[512];size_t length=0;int status=0;meeting_time_response response;
 if(!client->fetch(client->ctx,nonce,body,sizeof body,&length,&status)||client->cancelled(client->ctx)||
    !meeting_time_response_parse(body,length,status,key_id,&response)||!client->uptime_ms(client->ctx,&received)){
  meeting_time_challenge_cancel(challenge);return client->cancelled(client->ctx)?MEETING_TIME_CLIENT_PAUSED:MEETING_TIME_CLIENT_FAILED;
 }
 verify_context v={client,&anchor->key,started};
 bool applied=meeting_time_proof_apply(challenge,response.message,sizeof response.message,response.signature,response.signature_len,
                                      received,verify_fresh,&v,clock_io,clock);
 if(client->cancelled(client->ctx))return MEETING_TIME_CLIENT_PAUSED;
 return applied?MEETING_TIME_CLIENT_OK:MEETING_TIME_CLIENT_FAILED;
}
