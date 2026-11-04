#include "meeting_credential_reset.h"
#include <string.h>

#define ORIGIN_OFFSET 8u
#define DEVICE_OFFSET 209u
#define CREDENTIAL_OFFSET 310u
#define EVENT_OFFSET 375u
#define CRC_OFFSET 476u
static uint32_t checksum(const uint8_t *p,unsigned n)
{
    uint32_t c=~0u;
    for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}
    return ~c;
}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t get32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static bool bounded(const char *s,unsigned capacity,unsigned min)
{
    unsigned i=0;while(i<capacity&&s[i])i++;
    return i>=min&&i<capacity;
}
static bool identifier(const char *s,unsigned capacity)
{
    if(!bounded(s,capacity,1))return false;
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='_'||*p=='-'))return false;
    return true;
}
static bool valid(const meeting_reset_record *v)
{
    if(!v||v->state<MEETING_RESET_PENDING||v->state>MEETING_RESET_REVOKED||
       !bounded(v->origin,sizeof(v->origin),9)||strncmp(v->origin,"https://",8)||
       !identifier(v->device_id,sizeof(v->device_id))||!identifier(v->event_id,sizeof(v->event_id))||
       !bounded(v->credential_id,sizeof(v->credential_id),64)||strlen(v->credential_id)!=64)return false;
    for(const unsigned char *p=(const unsigned char *)v->origin+8;*p;p++)
        if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'||*p==':'))return false;
    for(const unsigned char *p=(const unsigned char *)v->credential_id;*p;p++)
        if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f')))return false;
    return true;
}
bool meeting_reset_encode(const meeting_reset_record *v,uint8_t out[MEETING_RESET_RECORD_SIZE])
{
    if(!out||!valid(v))return false;
    memset(out,0,MEETING_RESET_RECORD_SIZE);memcpy(out,"MRV1",4);out[4]=(uint8_t)v->state;
    memcpy(out+ORIGIN_OFFSET,v->origin,strlen(v->origin));memcpy(out+DEVICE_OFFSET,v->device_id,strlen(v->device_id));
    memcpy(out+CREDENTIAL_OFFSET,v->credential_id,strlen(v->credential_id));memcpy(out+EVENT_OFFSET,v->event_id,strlen(v->event_id));
    put32(out+CRC_OFFSET,checksum(out,CRC_OFFSET));return true;
}
bool meeting_reset_decode(const uint8_t in[MEETING_RESET_RECORD_SIZE],meeting_reset_record *out)
{
    if(!in||!out||memcmp(in,"MRV1",4)||in[5]||in[6]||in[7]||get32(in+CRC_OFFSET)!=checksum(in,CRC_OFFSET))return false;
    meeting_reset_record v={.state=(meeting_reset_state)in[4]};
    memcpy(v.origin,in+ORIGIN_OFFSET,sizeof(v.origin));memcpy(v.device_id,in+DEVICE_OFFSET,sizeof(v.device_id));
    memcpy(v.credential_id,in+CREDENTIAL_OFFSET,sizeof(v.credential_id));memcpy(v.event_id,in+EVENT_OFFSET,sizeof(v.event_id));
    uint8_t canonical[MEETING_RESET_RECORD_SIZE];
    if(!valid(&v)||!meeting_reset_encode(&v,canonical)||memcmp(canonical,in,sizeof(canonical)))return false;
    *out=v;return true;
}
static bool identity_matches(const meeting_reset_record *a,const meeting_reset_record *b)
{return !strcmp(a->origin,b->origin)&&!strcmp(a->device_id,b->device_id)&&!strcmp(a->credential_id,b->credential_id)&&!strcmp(a->event_id,b->event_id);}
static int load(const meeting_reset_io *io,meeting_reset_record *out)
{
    uint8_t data[MEETING_RESET_RECORD_SIZE];int status=io->read(io->ctx,data);
    if(status!=0)return status==1?1:-1;
    return meeting_reset_decode(data,out)?0:-1;
}
static bool save(const meeting_reset_io *io,const meeting_reset_record *v)
{
    uint8_t data[MEETING_RESET_RECORD_SIZE],check[MEETING_RESET_RECORD_SIZE];
    return meeting_reset_encode(v,data)&&io->write_sync(io->ctx,data)&&io->read(io->ctx,check)==0&&!memcmp(data,check,sizeof(data));
}
meeting_reset_result meeting_reset_begin(const meeting_reset_io *io,const meeting_reset_record *identity)
{
    if(!io||!io->read||!io->write_sync||!io->credential_state||!io->clear_credentials||!valid(identity)||identity->state!=MEETING_RESET_PENDING)return MEETING_RESET_ERROR;
    meeting_reset_record record;int status=load(io,&record);
    if(status<0)return MEETING_RESET_ERROR;
    if(status==1){
        if(io->credential_state(io->ctx,identity)!=0)return MEETING_RESET_CONFLICT;
        record=*identity;if(!save(io,&record))return MEETING_RESET_ERROR;
    }else {
        if(!identity_matches(&record,identity))return MEETING_RESET_CONFLICT;
        /* A prior NVS commit can fail while leaving staged bytes readable.
         * Recommit and verify before trusting any existing state. */
        if(!save(io,&record))return MEETING_RESET_ERROR;
    }
    if(record.state!=MEETING_RESET_PENDING)return MEETING_RESET_OK;
    int current=io->credential_state(io->ctx,&record);if(current<0)return MEETING_RESET_CONFLICT;
    /* A failed commit can leave an erase visible only in the NVS staging
     * area. Recommit even if a previous attempt appears already absent. */
    if(!io->clear_credentials(io->ctx)||io->credential_state(io->ctx,&record)!=1)return MEETING_RESET_ERROR;
    record.state=MEETING_RESET_LOCAL_CLEARED;
    return save(io,&record)?MEETING_RESET_OK:MEETING_RESET_ERROR;
}
meeting_reset_result meeting_reset_ack(const meeting_reset_io *io,const meeting_reset_record *identity,bool verified)
{
    if(!io||!io->read||!io->write_sync||!identity||!valid(identity)||!verified)return MEETING_RESET_ERROR;
    meeting_reset_record record;int status=load(io,&record);
    if(status==1)return MEETING_RESET_NOT_FOUND;
    if(status<0)return MEETING_RESET_ERROR;
    if(!identity_matches(&record,identity))return MEETING_RESET_CONFLICT;
    if(!save(io,&record))return MEETING_RESET_ERROR;
    if(record.state==MEETING_RESET_REVOKED)return MEETING_RESET_OK;
    if(record.state!=MEETING_RESET_LOCAL_CLEARED)return MEETING_RESET_ERROR;
    record.state=MEETING_RESET_REVOKED;return save(io,&record)?MEETING_RESET_OK:MEETING_RESET_ERROR;
}
