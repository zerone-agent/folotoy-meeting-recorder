#include "meeting_retirement.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t get32(const uint8_t *p){uint32_t n=0;for(unsigned i=0;i<4;i++)n|=(uint32_t)p[i]<<(8*i);return n;}
static uint32_t crc32(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static bool valid(const meeting_retired *v){if(!v||!v->file_id)return false;unsigned any=0;for(unsigned i=0;i<16;i++)any|=v->meeting_id[i];return any!=0;}
bool meeting_retired_encode(const meeting_retired *v,uint8_t out[MEETING_RETIRED_SIZE])
{
    if(!out||!valid(v))return false;
    memset(out,0,MEETING_RETIRED_SIZE);memcpy(out,"MRT2",4);put32(out+4,v->file_id);memcpy(out+8,v->meeting_id,16);
    out[24]=v->complete?1:0;put32(out+28,crc32(out,28));return true;
}
bool meeting_retired_decode(const uint8_t data[MEETING_RETIRED_SIZE],meeting_retired *out)
{
    if(!data||!out||memcmp(data,"MRT2",4)||get32(data+28)!=crc32(data,28)||data[24]>1||data[25]||data[26]||data[27])return false;
    meeting_retired v={.file_id=get32(data+4),.complete=data[24]!=0};memcpy(v.meeting_id,data+8,16);if(!valid(&v))return false;*out=v;return true;
}
meeting_retirement_result meeting_retirement_run(const meeting_retired *identity,bool fresh_mount,const meeting_retirement_io *io)
{
    if(!valid(identity)||!io||!io->cancelled||!io->read_marker||!io->write_marker_sync||!io->verify_completed||
       !io->files_absent||!io->erase_part_sync)return MEETING_RETIRE_ERROR;
    if(io->cancelled(io->ctx))return MEETING_RETIRE_PAUSED;
    uint8_t expected[MEETING_RETIRED_SIZE],actual[MEETING_RETIRED_SIZE];meeting_retired decoded;
    meeting_retired pending=*identity;pending.complete=false;
    if(!meeting_retired_encode(&pending,expected))return MEETING_RETIRE_ERROR;
    int status=io->read_marker(io->ctx,actual);
    if(status==0){
        if(!meeting_retired_decode(actual,&decoded)||decoded.file_id!=identity->file_id||
           memcmp(decoded.meeting_id,identity->meeting_id,sizeof(decoded.meeting_id)))return MEETING_RETIRE_ERROR;
        if(decoded.complete)return io->files_absent(io->ctx)?MEETING_RETIRE_DONE:MEETING_RETIRE_ERROR;
    }else if(status==1){
        if(!fresh_mount||!io->files_absent(io->ctx)||!io->verify_completed(io->ctx,identity))return MEETING_RETIRE_ERROR;
    }else return MEETING_RETIRE_ERROR;
    if(io->cancelled(io->ctx))return MEETING_RETIRE_PAUSED;
    if(!io->files_absent(io->ctx)||!io->write_marker_sync(io->ctx,expected)||
       io->read_marker(io->ctx,actual)!=0||memcmp(actual,expected,sizeof(actual)))return MEETING_RETIRE_ERROR;
    for(int part=0;part<MEETING_RETIRE_PARTS;part++){
        if(io->cancelled(io->ctx))return MEETING_RETIRE_PAUSED;
        if(!io->files_absent(io->ctx)||!io->erase_part_sync(io->ctx,(meeting_retire_part)part))return MEETING_RETIRE_ERROR;
    }
    if(!io->files_absent(io->ctx))return MEETING_RETIRE_ERROR;
    pending.complete=true;
    if(!meeting_retired_encode(&pending,expected)||!io->write_marker_sync(io->ctx,expected)||
       io->read_marker(io->ctx,actual)!=0||memcmp(actual,expected,sizeof(actual)))return MEETING_RETIRE_ERROR;
    return MEETING_RETIRE_DONE;
}
