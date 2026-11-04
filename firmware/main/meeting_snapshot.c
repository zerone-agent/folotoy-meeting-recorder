#include "meeting_snapshot.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void put64(uint8_t *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t get32(const uint8_t *p){uint32_t n=0;for(unsigned i=0;i<4;i++)n|=(uint32_t)p[i]<<(8*i);return n;}
static uint64_t get64(const uint8_t *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static uint32_t crc32(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u & (0u-(c&1u)));}return ~c;}
static bool valid(const meeting_snapshot *v){uint8_t id=0,h=0;for(unsigned i=0;i<16;i++)id|=v->meeting_id[i];for(unsigned i=0;i<32;i++)h|=v->source_sha256[i];return v->generation>0 && v->valid_end>=98 && v->valid_end<=20000000 && v->frames>0 && v->frames<=90000 && v->reason<=3 && id && h;}
bool meeting_snapshot_encode(const meeting_snapshot *v,uint8_t out[MEETING_SNAPSHOT_SIZE]){
 if(!v||!out||!valid(v))return false;
 memset(out,0,MEETING_SNAPSHOT_SIZE);memcpy(out,"MSP1",4);put32(out+4,MEETING_PROJECTION_V1);put64(out+8,v->generation);put64(out+16,v->valid_end);put32(out+24,v->frames);memcpy(out+28,v->meeting_id,16);memcpy(out+44,v->source_sha256,32);out[76]=v->reason;put32(out+84,crc32(out,84));return true;
}
bool meeting_snapshot_decode(const uint8_t in[MEETING_SNAPSHOT_SIZE],meeting_snapshot *out){
 if(!in||!out||memcmp(in,"MSP1",4)||get32(in+4)!=MEETING_PROJECTION_V1||get32(in+84)!=crc32(in,84))return false;
 for(unsigned i=77;i<84;i++)if(in[i])return false;
 meeting_snapshot v={0};v.generation=get64(in+8);v.valid_end=get64(in+16);v.frames=get32(in+24);memcpy(v.meeting_id,in+28,16);memcpy(v.source_sha256,in+44,32);v.reason=in[76];if(!valid(&v))return false;*out=v;return true;
}

meeting_snapshot_status meeting_snapshot_load(const meeting_snapshot_io *io, meeting_snapshot *out, unsigned *slot)
{
 if (!io || !io->read || !out || !slot) return MEETING_SNAPSHOT_ERROR;
 uint8_t raw[2][MEETING_SNAPSHOT_SIZE];meeting_snapshot values[2];int state[2];
 for (unsigned i=0;i<2;i++){int r=io->read(io->ctx,i,raw[i]);if(r<0 || r>1)return MEETING_SNAPSHOT_ERROR;state[i]=r==1?0:meeting_snapshot_decode(raw[i],&values[i])?1:2;}
 if(!state[0]&&!state[1])return MEETING_SNAPSHOT_EMPTY;
 if(state[0]==2 || state[1]==2){if(state[0]==1){*out=values[0];*slot=0;}else if(state[1]==1){*out=values[1];*slot=1;}return MEETING_SNAPSHOT_DEGRADED;}
 unsigned pick=state[1]==1 && (state[0]!=1 || values[1].generation>values[0].generation)?1:0;
 if(state[0]==1 && state[1]==1 && values[0].generation==values[1].generation && memcmp(raw[0],raw[1],MEETING_SNAPSHOT_SIZE))return MEETING_SNAPSHOT_ERROR;
 *out=values[pick];*slot=pick;return MEETING_SNAPSHOT_OK;
}

bool meeting_snapshot_commit(const meeting_snapshot_io *io,const meeting_snapshot *next)
{
 if(!io||!io->write_sync||!next)return false;
 meeting_snapshot current;unsigned active=0;meeting_snapshot_status status=meeting_snapshot_load(io,&current,&active);
 if(status!=MEETING_SNAPSHOT_OK && status!=MEETING_SNAPSHOT_EMPTY)return false;
 if(status==MEETING_SNAPSHOT_EMPTY ? next->generation!=1 : current.generation==UINT64_MAX || next->generation!=current.generation+1 || memcmp(current.meeting_id,next->meeting_id,16))return false;
 uint8_t raw[MEETING_SNAPSHOT_SIZE];if(!meeting_snapshot_encode(next,raw))return false;
 unsigned target=status==MEETING_SNAPSHOT_EMPTY?0:1-active;
 if(!io->write_sync(io->ctx,target,raw))return false;
 uint8_t check[MEETING_SNAPSHOT_SIZE];return io->read(io->ctx,target,check)==0 && !memcmp(raw,check,sizeof raw);
}
