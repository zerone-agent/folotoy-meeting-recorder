#include "meeting_time_signature.h"
#include "meeting_time_proof.h"
#include "meeting_time_anchor.h"
#include "meeting_time_response.h"
#include "meeting_time_client.h"
#include "mbedtls/sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {int64_t floor;int writes,sets;} time_store;
static int read_floor(void *ctx,int64_t *floor){*floor=((time_store *)ctx)->floor;return 0;}
static bool commit_floor(void *ctx,int64_t floor){time_store *s=ctx;s->writes++;s->floor=floor;return true;}
static bool set_time(void *ctx,int64_t epoch){time_store *s=ctx;assert(epoch==1800000000);s->sets++;return true;}
typedef struct {uint8_t data[MEETING_TIME_ANCHOR_SIZE];bool present;} anchor_store;
static int anchor_read(void *ctx,uint8_t *out){anchor_store *s=ctx;if(!s->present)return 1;memcpy(out,s->data,sizeof s->data);return 0;}
static bool anchor_write(void *ctx,const uint8_t *data){anchor_store *s=ctx;memcpy(s->data,data,sizeof s->data);s->present=true;return true;}
static bool anchor_authorized(void *ctx,const meeting_time_anchor *a){(void)ctx;return a!=NULL;}
typedef struct {const uint8_t *json;size_t len;bool cancelled;int status;int calls;uint64_t now;} client_fixture;
static bool random_nonce(void *ctx,uint8_t nonce[32]){(void)ctx;memset(nonce,7,32);return true;}
static bool uptime(void *ctx,uint64_t *out){client_fixture *f=ctx;*out=f->now;return true;}
static bool is_cancelled(void *ctx){return ((client_fixture *)ctx)->cancelled;}
static bool fetch(void *ctx,const uint8_t nonce[32],char *out,size_t cap,size_t *length,int *status){
 client_fixture *f=ctx;for(size_t i=0;i<32;i++)assert(nonce[i]==7);f->calls++;if(f->len>cap)return false;
 memcpy(out,f->json,f->len);*length=f->len;*status=f->status;f->now+=100;return true;
}
static size_t load(const char *dir,const char *name,uint8_t *out,size_t cap){char path[1024];assert(snprintf(path,sizeof path,"%s/%s",dir,name)>0);FILE *f=fopen(path,"rb");assert(f);size_t n=fread(out,1,cap,f);assert(!ferror(f)&&fgetc(f)==EOF);assert(fclose(f)==0);return n;}
int main(int argc,char **argv){
 assert(argc==2);meeting_time_public_key key;uint8_t msg[77],sig[73];
 assert(load(argv[1],"point.bin",key.public_key,sizeof key.public_key)==65);
 /* fread at exact capacity doesn't necessarily set EOF: the vector loader is
  * called with one spare byte for variable-length inputs. */
 size_t n=load(argv[1],"message.bin",msg,sizeof msg),s=load(argv[1],"signature.bin",sig,sizeof sig);
 assert(n==MEETING_TIME_PROOF_SIZE&&s>=8&&s<=72);
 uint8_t json[513],key_id[32];size_t json_len=load(argv[1],"response.json",json,sizeof json);
 assert(mbedtls_sha256(key.public_key,sizeof key.public_key,key_id,0)==0);
 meeting_time_response response;
 assert(meeting_time_response_parse((const char *)json,json_len,200,key_id,&response));
 assert(response.signature_len==s&&!memcmp(response.message,msg,n)&&!memcmp(response.signature,sig,s));
 key_id[0]^=1;assert(!meeting_time_response_parse((const char *)json,json_len,200,key_id,&response));key_id[0]^=1;
 assert(meeting_time_public_key_valid(NULL,&key));
 anchor_store saved={0};meeting_time_anchor_session session={0};meeting_time_anchor anchor={0},loaded;
 meeting_time_anchor_io anchor_io={&saved,anchor_read,anchor_write};memcpy(anchor.origin_sha256,msg+36,32);anchor.key=key;
 assert(meeting_time_anchor_install(&session,&anchor_io,&anchor,anchor_authorized,NULL,meeting_time_public_key_valid,NULL)==MEETING_ANCHOR_OK);
 assert(meeting_time_anchor_load(&session,&anchor_io,msg+36,meeting_time_public_key_valid,NULL,&loaded)==MEETING_ANCHOR_OK);
 assert(!memcmp(loaded.key.public_key,key.public_key,sizeof key.public_key));
 assert(meeting_time_signature_verify(&key,msg,n,sig,s));
 client_fixture fixture={.json=json,.len=json_len,.status=200,.now=100};
 meeting_time_client_io client={&fixture,random_nonce,uptime,is_cancelled,fetch};
 time_store client_store={.floor=1799999999};meeting_clock_io client_io={&client_store,read_floor,commit_floor,set_time};
 meeting_clock client_clock={0};meeting_time_challenge client_challenge={0};
 assert(meeting_time_client_run(&client_challenge,"https://meeting.example",&loaded,&client,&client_io,&client_clock)==MEETING_TIME_CLIENT_OK);
 assert(client_clock.trusted&&client_store.writes==1&&client_store.sets==1&&fixture.calls==1);
 client_clock=(meeting_clock){0};client_store=(time_store){.floor=1799999999};client_challenge=(meeting_time_challenge){0};fixture.status=503;
 assert(meeting_time_client_run(&client_challenge,"https://meeting.example",&loaded,&client,&client_io,&client_clock)==MEETING_TIME_CLIENT_FAILED);
 assert(!client_clock.trusted&&!client_store.writes);
 fixture.status=200;int prior_calls=fixture.calls;
 assert(meeting_time_client_run(&client_challenge,"https://meeting.example",&loaded,&client,&client_io,&client_clock)==MEETING_TIME_CLIENT_FAILED);
 assert(fixture.calls==prior_calls); /* A failed attempt cannot reuse its nonce. */
 client_challenge=(meeting_time_challenge){0};
 assert(meeting_time_client_run(&client_challenge,"https://other.example",&loaded,&client,&client_io,&client_clock)==MEETING_TIME_CLIENT_FAILED);
 fixture.cancelled=true;
 assert(meeting_time_client_run(&client_challenge,"https://meeting.example",&loaded,&client,&client_io,&client_clock)==MEETING_TIME_CLIENT_PAUSED);
 assert(!client_store.writes);
 time_store store={.floor=1799999999};meeting_clock_io io={&store,read_floor,commit_floor,set_time};
 meeting_clock clock={0};meeting_time_challenge challenge={0};
 assert(meeting_time_challenge_begin(&challenge,msg+4,msg+36,100));
 assert(meeting_time_proof_apply(&challenge,msg,n,sig,s,200,meeting_time_signature_verify,&key,&io,&clock));
 assert(clock.trusted&&store.writes==1&&store.sets==1);
 assert(!meeting_time_proof_apply(&challenge,msg,n,sig,s,201,meeting_time_signature_verify,&key,&io,&clock));
 msg[4]^=1;assert(!meeting_time_signature_verify(&key,msg,n,sig,s));msg[4]^=1;
 msg[36]^=1;assert(!meeting_time_signature_verify(&key,msg,n,sig,s));msg[36]^=1;
 sig[s-1]^=1;assert(!meeting_time_signature_verify(&key,msg,n,sig,s));sig[s-1]^=1;
 sig[s]=0;assert(!meeting_time_signature_verify(&key,msg,n,sig,s+1));
 key.public_key[2]^=1;assert(!meeting_time_public_key_valid(NULL,&key));assert(!meeting_time_signature_verify(&key,msg,n,sig,s));
 anchor.key=key;assert(meeting_time_anchor_install(&session,&anchor_io,&anchor,anchor_authorized,NULL,meeting_time_public_key_valid,NULL)==MEETING_ANCHOR_ERROR);
 return 0;
}
