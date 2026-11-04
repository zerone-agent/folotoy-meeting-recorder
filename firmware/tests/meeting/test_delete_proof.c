#define _POSIX_C_SOURCE 200809L
#include "meeting_delete_proof.h"
#include <mbedtls/sha256.h>
#include <assert.h>
#include <string.h>
void mbedtls_sha256_init(mbedtls_sha256_context *c){c->value=1;}
int mbedtls_sha256_starts(mbedtls_sha256_context *c,int x){(void)x;c->value=1;return 0;}
int mbedtls_sha256_update(mbedtls_sha256_context *c,const unsigned char *p,size_t n){for(size_t i=0;i<n;i++)c->value=c->value*33+p[i];return 0;}
int mbedtls_sha256_finish(mbedtls_sha256_context *c,unsigned char out[32]){for(int i=0;i<32;i++)out[i]=(unsigned char)(c->value>>((i%4)*8));return 0;}
void mbedtls_sha256_free(mbedtls_sha256_context *c){(void)c;}
typedef struct {uint8_t data[MEETING_CONFIRMATION_SIZE];bool present,fail;} fixture;
static int rd(void *ctx,uint8_t *out){fixture *f=ctx;if(!f->present)return 1;memcpy(out,f->data,sizeof(f->data));return 0;}
static bool wr(void *ctx,const uint8_t *in){fixture *f=ctx;if(f->fail)return false;memcpy(f->data,in,sizeof(f->data));f->present=true;return true;}
static bool cancelled(void *ctx){return *(bool *)ctx;}
int main(void){
 uint8_t data[100]={0};memcpy(data,"SCR1",4);data[4]=1;data[20]=1;data[96]=3;data[97]=0xf8;data[98]=0xff;data[99]=0xfe;
 FILE *f=tmpfile();assert(f&&fwrite(data,1,sizeof(data),f)==sizeof(data)&&fflush(f)==0);
 meeting_snapshot snap={.generation=1,.valid_end=100,.frames=1,.meeting_id={1},.reason=0};assert(meeting_snapshot_source_hash(f,snap.source_sha256));
 fixture state={0};meeting_confirmation_io io={&state,rd,wr};meeting_manifest expected;bool stop=true;
 assert(!meeting_delete_proof_prepare(7,f,&snap,false,0,0,&io,cancelled,&stop)&&!state.present);
 stop=false;state.fail=true;assert(!meeting_delete_proof_prepare(7,f,&snap,false,0,0,&io,cancelled,&stop)&&!state.present);
 state.fail=false;assert(meeting_delete_proof_prepare(7,f,&snap,false,0,0,&io,cancelled,&stop));assert(fclose(f)==0);
 assert(meeting_delete_proof_load(7,&snap,&io,&expected)&&expected.bytes==100&&expected.reason==0);
 assert(!meeting_delete_proof_load(8,&snap,&io,&expected));meeting_snapshot other=snap;other.meeting_id[0]=2;
 assert(!meeting_delete_proof_load(7,&other,&io,&expected));state.data[56]^=1;
 assert(!meeting_delete_proof_load(7,&snap,&io,&expected));return 0;
}
