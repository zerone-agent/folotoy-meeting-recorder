#define _POSIX_C_SOURCE 200809L
#include "meeting_cleanup_files.h"
#include "meeting_upload.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {uint8_t data[MEETING_CONFIRMATION_SIZE];int present;} slot;
typedef struct {int closed,space,valid;} env;
static meeting_manifest expected;
static int read_slot(void *ctx,uint8_t out[MEETING_CONFIRMATION_SIZE]){slot *s=ctx;if(!s->present)return 1;memcpy(out,s->data,sizeof(s->data));return 0;}
static bool write_slot(void *ctx,const uint8_t data[MEETING_CONFIRMATION_SIZE]){slot *s=ctx;memcpy(s->data,data,sizeof(s->data));s->present=1;return true;}
bool meeting_upload_manifest_cancel(FILE *f,const meeting_snapshot *s,bool has_time,int64_t time,uint8_t trust,
                                   meeting_manifest *out,meeting_cancel_fn cancelled,void *ctx)
{
    (void)s;(void)has_time;(void)time;(void)trust;env *e=ctx;
    if(cancelled(ctx)||!e->valid||fgetc(f)!='x')return false;
    *out=expected;return true;
}
static bool cancelled(void *ctx){(void)ctx;return false;}
static bool close_readers(void *ctx){((env *)ctx)->closed++;return true;}
static bool read_space(void *ctx){((env *)ctx)->space++;return true;}
int main(void)
{
    char dir[]="/tmp/meeting-cleanup-files.XXXXXX";int fd=mkstemp(dir);
    assert(fd>=0&&close(fd)==0&&unlink(dir)==0&&mkdir(dir,0700)==0);
    char audio[128],index[128];assert(snprintf(audio,sizeof(audio),"%s/1.dat",dir)>0);assert(snprintf(index,sizeof(index),"%s/1.idx",dir)>0);
    FILE *f=fopen(audio,"wb");assert(f&&fputs("x",f)>=0&&fclose(f)==0);f=fopen(index,"wb");assert(f&&fputs("y",f)>=0&&fclose(f)==0);
    meeting_confirmation receipt={.file_id=1,.source_sha256={1},.manifest={.count=1,.bytes=100,.digest={2},.reason=3}};
    memcpy(receipt.manifest.id,"12345678-1234-4123-8123-123456789abc",37);expected=receipt.manifest;
    meeting_snapshot snapshot={.valid_end=100,.reason=3,.source_sha256={1},
        .meeting_id={0x12,0x34,0x56,0x78,0x12,0x34,0x41,0x23,0x81,0x23,0x12,0x34,0x56,0x78,0x9a,0xbc}};
    slot ack={0},intent={0};meeting_confirmation_io a={&ack,read_slot,write_slot},i={&intent,read_slot,write_slot};
    assert(meeting_confirmation_commit(&a,&receipt));env environment={0};
    meeting_cleanup_files files={audio,index,cancelled,close_readers,read_space,&environment};meeting_cleanup_io io;
    assert(meeting_cleanup_files_bind(&files,&io));
    assert(meeting_cleanup_run(1,&snapshot,&a,&i,&io)==MEETING_CLEANUP_ERROR&&!intent.present&&access(audio,F_OK)==0);
    environment.valid=1;assert(meeting_cleanup_run(1,&snapshot,&a,&i,&io)==MEETING_CLEANUP_DONE);
    assert(intent.present&&environment.closed==1&&environment.space==1&&access(audio,F_OK)!=0&&access(index,F_OK)!=0);
    assert(meeting_cleanup_run(1,&snapshot,&a,&i,&io)==MEETING_CLEANUP_DONE);assert(rmdir(dir)==0);
    return 0;
}
