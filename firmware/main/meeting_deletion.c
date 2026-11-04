#include "meeting_deletion.h"
#include <string.h>
#include <limits.h>
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void put64(uint8_t *p,uint64_t v){for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t get32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static uint64_t get64(const uint8_t *p){uint64_t v=0;for(unsigned i=0;i<8;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static uint32_t crc32(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static bool valid(const meeting_deletion *v){
 if(!v||!v->generation||!v->file_id||(unsigned)v->remote>MEETING_DELETE_REMOTE_COMMITTED||
    (v->legacy_local_only&&v->empty_local_only)||
    ((v->legacy_local_only||v->empty_local_only)&&v->remote!=MEETING_DELETE_REMOTE_PENDING))return false;
 unsigned m=0,o=0;for(unsigned i=0;i<16;i++){m|=v->meeting_id[i];o|=v->operation_id[i];}return m&&o;
}
bool meeting_deletion_encode(const meeting_deletion *v,uint8_t out[MEETING_DELETION_SIZE]){
 if(!out||!valid(v))return false;
 memset(out,0,MEETING_DELETION_SIZE);memcpy(out,(v->legacy_local_only||v->empty_local_only)?"MDL2":"MDL1",4);put64(out+4,v->generation);put32(out+12,v->file_id);
 memcpy(out+16,v->meeting_id,16);memcpy(out+32,v->operation_id,16);out[48]=v->local_done?1:0;out[49]=(uint8_t)v->remote;
 if(v->legacy_local_only)out[50]=1;else if(v->empty_local_only)out[50]=2;
 put32(out+60,crc32(out,60));return true;
}
bool meeting_deletion_decode(const uint8_t in[MEETING_DELETION_SIZE],meeting_deletion *out){
 if(!in||!out||(memcmp(in,"MDL1",4)&&memcmp(in,"MDL2",4))||in[48]>1||in[49]>MEETING_DELETE_REMOTE_COMMITTED||get32(in+60)!=crc32(in,60))return false;
 bool local_only=!memcmp(in,"MDL2",4);if(local_only?(in[50]!=1&&in[50]!=2):in[50]!=0)return false;
 for(unsigned i=51;i<60;i++)if(in[i])return false;
 meeting_deletion v={.generation=get64(in+4),.file_id=get32(in+12),.local_done=in[48]!=0,.legacy_local_only=local_only&&in[50]==1,.empty_local_only=local_only&&in[50]==2,.remote=(meeting_delete_remote)in[49]};
 memcpy(v.meeting_id,in+16,16);memcpy(v.operation_id,in+32,16);if(!valid(&v))return false;*out=v;return true;
}
meeting_deletion_status meeting_deletion_load(const meeting_deletion_io *io,meeting_deletion *out,unsigned *slot){
 if(!io||!io->read||!out||!slot)return MEETING_DELETION_IO_ERROR;
 uint8_t raw[2][MEETING_DELETION_SIZE];meeting_deletion v[2];int status[2];
 for(unsigned i=0;i<2;i++){int r=io->read(io->ctx,i,raw[i]);if(r<0||r>1)return MEETING_DELETION_IO_ERROR;status[i]=r==1?0:meeting_deletion_decode(raw[i],&v[i])?1:2;}
 if(!status[0]&&!status[1])return MEETING_DELETION_EMPTY;
 if(status[0]==2||status[1]==2)return MEETING_DELETION_DAMAGED;
 if(status[0]&&status[1]&&v[0].generation==v[1].generation&&memcmp(raw[0],raw[1],MEETING_DELETION_SIZE))return MEETING_DELETION_DAMAGED;
 unsigned pick=status[1]&&(!status[0]||v[1].generation>v[0].generation)?1:0;
 if(status[0]&&status[1]&&v[0].generation!=v[1].generation){
  const meeting_deletion *newer=&v[pick],*older=&v[1-pick];
  if(older->generation==UINT64_MAX||newer->generation!=older->generation+1||newer->file_id!=older->file_id||
     memcmp(newer->meeting_id,older->meeting_id,16)||memcmp(newer->operation_id,older->operation_id,16)||newer->legacy_local_only!=older->legacy_local_only||newer->empty_local_only!=older->empty_local_only||
     (older->local_done&&!newer->local_done)||
     (older->remote!=MEETING_DELETE_REMOTE_PENDING&&newer->remote!=older->remote))return MEETING_DELETION_DAMAGED;
 }
 *out=v[pick];*slot=pick;return MEETING_DELETION_OK;
}
bool meeting_deletion_commit(const meeting_deletion_io *io,const meeting_deletion *next){
 if(!io||!io->write_sync||!next||!valid(next))return false;
 meeting_deletion current;unsigned slot=0;meeting_deletion_status status=meeting_deletion_load(io,&current,&slot);
 if(status!=MEETING_DELETION_EMPTY&&status!=MEETING_DELETION_OK)return false;
 if(status==MEETING_DELETION_EMPTY){if(next->generation!=1||next->local_done||next->remote!=MEETING_DELETE_REMOTE_PENDING)return false;}
 else if(current.generation==UINT64_MAX||next->generation!=current.generation+1||next->file_id!=current.file_id||
         memcmp(next->meeting_id,current.meeting_id,16)||memcmp(next->operation_id,current.operation_id,16)||next->legacy_local_only!=current.legacy_local_only||next->empty_local_only!=current.empty_local_only||
         (current.local_done&&!next->local_done)||
         (current.remote!=MEETING_DELETE_REMOTE_PENDING&&next->remote!=current.remote))return false;
 uint8_t bytes[MEETING_DELETION_SIZE],check[MEETING_DELETION_SIZE];if(!meeting_deletion_encode(next,bytes))return false;
 unsigned target=status==MEETING_DELETION_EMPTY?0:1-slot;
 if(!io->write_sync(io->ctx,target,bytes))return false;
 return io->read(io->ctx,target,check)==0&&!memcmp(bytes,check,sizeof(bytes));
}
