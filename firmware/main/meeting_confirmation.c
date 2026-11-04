#include "meeting_confirmation.h"
#include "meeting_upload.h"
#include <string.h>

static void put32(uint8_t *p, uint32_t v) { for (unsigned i=0;i<4;i++) p[i]=(uint8_t)(v>>(8*i)); }
static void put64(uint8_t *p, uint64_t v) { for (unsigned i=0;i<8;i++) p[i]=(uint8_t)(v>>(8*i)); }
static uint32_t get32(const uint8_t *p) { uint32_t v=0; for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i); return v; }
static uint64_t get64(const uint8_t *p) { uint64_t v=0; for(unsigned i=0;i<8;i++)v|=(uint64_t)p[i]<<(8*i); return v; }
static uint32_t crc32(const uint8_t *p,size_t n) { uint32_t c=~0u; for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c; }
static bool valid(const meeting_confirmation *v)
{
    if (!v || !v->file_id || !meeting_receipt_matches(&v->manifest,&v->manifest,true) ||
        (!v->manifest.recorded_at_present && v->manifest.recorded_at != 0)) return false;
    unsigned source=0, digest=0;
    for(unsigned i=0;i<32;i++){source|=v->source_sha256[i];digest|=v->manifest.digest[i];}
    return source && digest;
}
bool meeting_confirmation_matches_snapshot(const meeting_confirmation *v,const meeting_snapshot *snapshot)
{
    if(!valid(v)||!snapshot||memcmp(v->source_sha256,snapshot->source_sha256,32)||
       v->manifest.bytes!=snapshot->valid_end||v->manifest.reason!=snapshot->reason||
       v->manifest.count!=(snapshot->valid_end+MEETING_UPLOAD_CHUNK_SIZE-1)/MEETING_UPLOAD_CHUNK_SIZE)return false;
    static const char hex[]="0123456789abcdef";char uuid[37];unsigned p=0;
    for(unsigned i=0;i<16;i++){
        if(i==4||i==6||i==8||i==10)uuid[p++]='-';
        uuid[p++]=hex[snapshot->meeting_id[i]>>4];uuid[p++]=hex[snapshot->meeting_id[i]&15];
    }
    uuid[p]='\0';return !memcmp(uuid,v->manifest.id,sizeof(uuid));
}
bool meeting_confirmation_encode(const meeting_confirmation *v,uint8_t out[MEETING_CONFIRMATION_SIZE])
{
    if(!out||!valid(v))return false;
    memset(out,0,MEETING_CONFIRMATION_SIZE);memcpy(out,"MCF1",4);put32(out+4,v->file_id);
    memcpy(out+8,v->manifest.id,36);put32(out+44,v->manifest.count);put64(out+48,v->manifest.bytes);
    memcpy(out+56,v->manifest.digest,32);memcpy(out+88,v->source_sha256,32);
    put64(out+120,(uint64_t)v->manifest.recorded_at);out[128]=v->manifest.recorded_at_present?1:0;
    out[129]=v->manifest.reason;out[130]=v->manifest.time_trust;put32(out+136,crc32(out,136));return true;
}
bool meeting_confirmation_decode(const uint8_t in[MEETING_CONFIRMATION_SIZE],meeting_confirmation *out)
{
    if(!in||!out||memcmp(in,"MCF1",4)||get32(in+136)!=crc32(in,136)||in[128]>1)return false;
    for(unsigned i=131;i<136;i++)if(in[i])return false;
    meeting_confirmation v={0};v.file_id=get32(in+4);memcpy(v.manifest.id,in+8,36);
    v.manifest.count=get32(in+44);v.manifest.bytes=get64(in+48);memcpy(v.manifest.digest,in+56,32);
    memcpy(v.source_sha256,in+88,32);uint64_t t=get64(in+120);if(t>INT64_MAX)return false;
    v.manifest.recorded_at=(int64_t)t;v.manifest.recorded_at_present=in[128]!=0;
    v.manifest.reason=in[129];v.manifest.time_trust=in[130];if(!valid(&v))return false;*out=v;return true;
}
meeting_confirmation_status meeting_confirmation_load(const meeting_confirmation_io *io,meeting_confirmation *out)
{
    if(!io||!io->read||!out)return MEETING_CONFIRMATION_ERROR;
    uint8_t data[MEETING_CONFIRMATION_SIZE];int r=io->read(io->ctx,data);
    if(r==1)return MEETING_CONFIRMATION_EMPTY;
    return r==0&&meeting_confirmation_decode(data,out)?MEETING_CONFIRMATION_OK:MEETING_CONFIRMATION_ERROR;
}
bool meeting_confirmation_commit(const meeting_confirmation_io *io,const meeting_confirmation *v)
{
    if(!io||!io->write_sync)return false;
    uint8_t expected[MEETING_CONFIRMATION_SIZE],actual[MEETING_CONFIRMATION_SIZE];
    if(!meeting_confirmation_encode(v,expected))return false;
    int r=io->read?io->read(io->ctx,actual):-1;
    if(r==0 && (!meeting_confirmation_decode(actual,&(meeting_confirmation){0})||memcmp(expected,actual,sizeof(actual))))return false;
    if((r!=0&&r!=1)||!io->write_sync(io->ctx,expected))return false;
    return io->read(io->ctx,actual)==0&&!memcmp(expected,actual,sizeof(actual));
}
