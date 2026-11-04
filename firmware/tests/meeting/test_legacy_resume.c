#include "meeting_upload_protocol.h"
#include "meeting_receipt_json.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static meeting_manifest expected;static int phase,first_put=-1,put_count;
bool meeting_upload_manifest_cancel(FILE *f,const meeting_snapshot *s,bool ht,int64_t rt,uint8_t trust,meeting_manifest *out,meeting_cancel_fn cancel,void *ctx){(void)f;(void)s;(void)ht;(void)rt;(void)trust;(void)cancel;(void)ctx;*out=expected;return true;}
bool meeting_upload_chunk(FILE *f,const meeting_snapshot *s,uint32_t seq,uint8_t *buf,size_t cap,size_t *len,uint8_t hash[32]){(void)f;(void)s;(void)seq;assert(cap>=3);memcpy(buf,"abc",3);memset(hash,7,32);*len=3;return true;}
bool meeting_receipt_json_matches(const char *j,size_t n,int status,const meeting_manifest *m){(void)j;(void)n;(void)m;return status==200;}
bool meeting_receipt_json_integrity_error(const char *j,size_t n,int status){(void)j;(void)n;(void)status;return false;}
bool meeting_repair_json_parse(const char *j,size_t n,int status,const meeting_manifest *m,uint8_t *bitmap,size_t size){(void)j;(void)n;(void)status;(void)m;(void)bitmap;(void)size;return false;}
bool meeting_chunk_window_json_parse(const char *j,size_t n,int status,const char *id,uint32_t first,meeting_remote_chunk entries[16]){(void)j;(void)n;(void)status;(void)id;(void)first;(void)entries;return false;}
bool meeting_legacy_chunk_window_json_parse(const char *j,size_t n,int status,const char *id,uint32_t first,meeting_remote_chunk entries[16]){(void)j;(void)n;(void)status;(void)id;(void)first;(void)entries;return false;}
static bool cancelled(void *ctx){(void)ctx;return false;}
static bool request(void *ctx,const char *method,const char *path,const char *type,const char *sha,const uint8_t *body,size_t body_len,int *status,char *response,size_t cap,size_t *len){
 (void)ctx;(void)type;(void)sha;(void)body;(void)body_len;(void)response;assert(cap>=2);*len=0;
 if(!strcmp(method,"GET")){if(strstr(path,"chunks-window")){*status=404;return true;}return false;}
 if(!strcmp(method,"PUT")){const char *last=strrchr(path,'/');assert(last);int seq=atoi(last+1);if(first_put<0)first_put=seq;put_count++;if(phase==0&&seq==10)return false;*status=200;return true;}
 if(strstr(path,"finalize")){*status=phase==1?409:200;return true;}
 *status=201;return true;
}
int main(void){memcpy(expected.id,"12345678-1234-4123-8123-123456789abc",37);expected.count=20;expected.bytes=100;expected.reason=3;memset(expected.digest,7,32);meeting_upload_transport t={NULL,cancelled,request};meeting_snapshot s={0};FILE *f=tmpfile();assert(f);
 assert(meeting_upload_run(f,&s,&expected,&t)==MEETING_UPLOAD_FAILED&&first_put==0&&put_count==11);
 expected.digest[0]=8;first_put=-1;put_count=0;
 assert(meeting_upload_run(f,&s,&expected,&t)==MEETING_UPLOAD_FAILED&&first_put==0&&put_count==11);
 expected.digest[0]=7;
 phase=1;first_put=-1;put_count=0;assert(meeting_upload_run(f,&s,&expected,&t)==MEETING_UPLOAD_FAILED&&first_put==10&&put_count==10);
 phase=2;first_put=-1;put_count=0;assert(meeting_upload_run(f,&s,&expected,&t)==MEETING_UPLOAD_CONFIRMED&&first_put==0&&put_count==20);
 first_put=-1;put_count=0;assert(meeting_upload_run(f,&s,&expected,&t)==MEETING_UPLOAD_CONFIRMED&&first_put==0&&put_count==20);fclose(f);return 0;}
