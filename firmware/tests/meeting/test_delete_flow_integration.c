#define _POSIX_C_SOURCE 200809L
#include "meeting_delete_workflow.h"
#include "meeting_delete_proof.h"
#include <mbedtls/sha256.h>
#include <assert.h>
#include <string.h>
void mbedtls_sha256_init(mbedtls_sha256_context *c){c->value=1;}
int mbedtls_sha256_starts(mbedtls_sha256_context *c,int x){(void)x;c->value=1;return 0;}
int mbedtls_sha256_update(mbedtls_sha256_context *c,const unsigned char *p,size_t n){for(size_t i=0;i<n;i++)c->value=c->value*33+p[i];return 0;}
int mbedtls_sha256_finish(mbedtls_sha256_context *c,unsigned char out[32]){for(int i=0;i<32;i++)out[i]=(unsigned char)(c->value>>((i%4)*8));return 0;}
void mbedtls_sha256_free(mbedtls_sha256_context *c){(void)c;}
typedef struct {uint8_t deletion[2][MEETING_DELETION_SIZE],proof[MEETING_CONFIRMATION_SIZE];bool has_deletion[2],has_proof,online;FILE *file;meeting_snapshot snap;meeting_confirmation_io proof_io;int requests;} fixture;
static int dr(void *ctx,unsigned slot,uint8_t *out){fixture *f=ctx;if(!f->has_deletion[slot])return 1;memcpy(out,f->deletion[slot],MEETING_DELETION_SIZE);return 0;}
static bool dw(void *ctx,unsigned slot,const uint8_t *in){fixture *f=ctx;memcpy(f->deletion[slot],in,MEETING_DELETION_SIZE);f->has_deletion[slot]=true;return true;}
static int pr(void *ctx,uint8_t *out){fixture *f=ctx;if(!f->has_proof)return 1;memcpy(out,f->proof,MEETING_CONFIRMATION_SIZE);return 0;}
static bool pw(void *ctx,const uint8_t *in){fixture *f=ctx;memcpy(f->proof,in,MEETING_CONFIRMATION_SIZE);f->has_proof=true;return true;}
static bool ensure(void *ctx,const meeting_deletion *d){fixture *f=ctx;meeting_manifest expected;if(f->has_proof)return meeting_delete_proof_load(d->file_id,&f->snap,&f->proof_io,&expected);return f->file&&meeting_delete_proof_prepare(d->file_id,f->file,&f->snap,false,0,0,&f->proof_io,NULL,NULL);}
static bool remove_local(void *ctx,const meeting_deletion *d){fixture *f=ctx;(void)d;if(f->file){assert(fclose(f->file)==0);f->file=NULL;}return true;}
static int cancel_remote(void *ctx,const meeting_deletion *d){fixture *f=ctx;meeting_manifest expected;assert(meeting_delete_proof_load(d->file_id,&f->snap,&f->proof_io,&expected));f->requests++;return f->online?1:0;}
int main(void){
 uint8_t b[100]={0};memcpy(b,"SCR1",4);b[4]=1;b[20]=1;b[96]=3;b[97]=0xf8;b[98]=0xff;b[99]=0xfe;
 fixture f={0};f.file=tmpfile();assert(f.file&&fwrite(b,1,sizeof(b),f.file)==sizeof(b)&&fflush(f.file)==0);
 f.snap=(meeting_snapshot){.generation=1,.valid_end=100,.frames=1,.meeting_id={1}};assert(meeting_snapshot_source_hash(f.file,f.snap.source_sha256));
 f.proof_io=(meeting_confirmation_io){&f,pr,pw};meeting_deletion_io storage={&f,dr,dw};meeting_delete_workflow_io io={&f,ensure,remove_local,cancel_remote};
 meeting_deletion intent={.generation=1,.file_id=7,.meeting_id={1},.operation_id={2}},loaded;unsigned slot;
 assert(meeting_deletion_commit(&storage,&intent));assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_PENDING&&!f.file&&f.has_proof);
 assert(meeting_deletion_load(&storage,&loaded,&slot)==MEETING_DELETION_OK&&loaded.local_done&&loaded.remote==MEETING_DELETE_REMOTE_PENDING);
 f.proof[0]^=1;f.online=true;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&f.requests==1);f.proof[0]^=1;
 assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_DONE&&f.requests==2);
 assert(meeting_deletion_load(&storage,&loaded,&slot)==MEETING_DELETION_OK&&loaded.remote==MEETING_DELETE_REMOTE_CANCELLED);return 0;
}
