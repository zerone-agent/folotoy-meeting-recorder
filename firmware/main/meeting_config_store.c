#include "meeting_config_store.h"
#include <stdlib.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t get32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static uint32_t checksum(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static bool valid_record(const uint8_t *data,size_t length)
{
    return data&&length>12&&length<=MEETING_CONFIG_RECORD_MAX&&!memcmp(data,"MCP1",4)&&
           get32(data+4)==length-12&&get32(data+8)==checksum(data+12,length-12);
}
static bool same_settings(const meeting_config *a,const meeting_config *b)
{
    return !strcmp(a->ssid,b->ssid)&&!strcmp(a->password,b->password)&&!strcmp(a->origin,b->origin)&&
           !strcmp(a->device_token,b->device_token)&&!strcmp(a->device_id,b->device_id)&&
           !strcmp(a->credential_id,b->credential_id)&&!strcmp(a->ca_pem,b->ca_pem)&&
           !memcmp(a->ca_sha256,b->ca_sha256,sizeof(a->ca_sha256))&&a->timezone_minutes==b->timezone_minutes;
}
meeting_config_store_status meeting_config_store_load(const meeting_config_store_io *io,meeting_ca_validate validate_ca,
                            void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *out)
{
    if(!io||!io->read||!io->write_sync||!validate_ca||!scratch||!out)return MEETING_CONFIG_INVALID;
    meeting_config_clear(out);
    size_t length=0;int status=io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&length);
    if(status==1)return MEETING_CONFIG_ABSENT;
    if(status!=0||!valid_record(scratch,length)||!meeting_config_parse(scratch+12,length-12,validate_ca,ca_ctx,out))return MEETING_CONFIG_INVALID;
    /* A prior NVS commit error may still leave a staged blob visible. Commit
     * again before using it and reread the exact record. */
    uint32_t crc=get32(scratch+8);
    if(!io->write_sync(io->ctx,scratch,length)){meeting_config_clear(out);return MEETING_CONFIG_INVALID;}
    size_t checked=0;
    if(io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&checked)!=0||checked!=length||
       !valid_record(scratch,checked)||get32(scratch+8)!=crc){meeting_config_clear(out);return MEETING_CONFIG_INVALID;}
    return MEETING_CONFIG_STORED;
}
bool meeting_config_store_commit(const meeting_config_store_io *io,const uint8_t *json,size_t length,
                                  bool authenticated,bool user_confirmed,meeting_ca_validate validate_ca,void *ca_ctx,
                                  uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *temporary)
{
    if(!io||!io->read||!io->write_sync||!json||!length||length>MEETING_CONFIG_MAX_BYTES||
       !authenticated||!user_confirmed||!validate_ca||!scratch||!temporary)return false;
    /* The input must not alias scratch: it survives the previous-record read. */
    uintptr_t begin=(uintptr_t)scratch,end=begin+MEETING_CONFIG_RECORD_MAX,input=(uintptr_t)json;
    if(end<begin||input>UINTPTR_MAX-length||(input<end&&input+length>begin))return false;
    meeting_config_clear(temporary);
    if(!meeting_config_parse(json,length,validate_ca,ca_ctx,temporary))return false;
    size_t previous=0;int status=io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&previous);
    if(status==0){
        if(!valid_record(scratch,previous)){meeting_config_clear(temporary);return false;}
        meeting_config *old=calloc(1,sizeof(*old));
        if(!old){meeting_config_clear(temporary);return false;}
        bool same=meeting_config_parse(scratch+12,previous-12,validate_ca,ca_ctx,old)&&same_settings(old,temporary);
        meeting_config_clear(old);free(old);meeting_config_clear(temporary);
        if(!same)return false;
        /* Keep the original committed config. Fresh UTC is only for the
         * separately verified clock bootstrap, not credential replacement. */
        uint32_t crc=get32(scratch+8);
        if(!io->write_sync(io->ctx,scratch,previous))return false;
        size_t checked=0;
        return io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&checked)==0&&checked==previous&&
               valid_record(scratch,checked)&&get32(scratch+8)==crc;
    }else if(status!=1){meeting_config_clear(temporary);return false;}
    meeting_config_clear(temporary);
    memcpy(scratch,"MCP1",4);put32(scratch+4,(uint32_t)length);put32(scratch+8,checksum(json,length));memcpy(scratch+12,json,length);
    /* Recommit identical records after an ambiguous prior failure. */
    if(!io->write_sync(io->ctx,scratch,length+12))return false;
    size_t readback=0;status=io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&readback);
    return status==0&&readback==length+12&&valid_record(scratch,readback)&&!memcmp(scratch+12,json,length);
}

bool meeting_config_store_compatible(const meeting_config_store_io *io,meeting_ca_validate validate_ca,
                                     void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],
                                     const meeting_config *expected,meeting_config *temporary)
{
    if(!io||!io->read||!validate_ca||!scratch||!expected||!temporary||expected==temporary)return false;
    size_t length=0;int status=io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&length);
    if(status==1)return true;
    if(status!=0||!valid_record(scratch,length))return false;
    bool same=meeting_config_parse(scratch+12,length-12,validate_ca,ca_ctx,temporary)&&same_settings(expected,temporary);
    meeting_config_clear(temporary);return same;
}

bool meeting_config_store_commit_preparsed(const meeting_config_store_io *io,const uint8_t *json,size_t length,
                                  meeting_ca_validate validate_ca,void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],
                                  const meeting_config *expected,meeting_config *temporary)
{
    if(!io||!io->read||!io->write_sync||!json||!length||length>MEETING_CONFIG_MAX_BYTES||
       !validate_ca||!scratch||!expected||!temporary||expected==temporary)return false;
    uintptr_t begin=(uintptr_t)scratch,end=begin+MEETING_CONFIG_RECORD_MAX,input=(uintptr_t)json;
    if(end<begin||input>UINTPTR_MAX-length||(input<end&&input+length>begin))return false;
    size_t previous=0;int status=io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&previous);
    if(status==0){
        if(!valid_record(scratch,previous))return false;
        bool same=meeting_config_parse(scratch+12,previous-12,validate_ca,ca_ctx,temporary)&&same_settings(expected,temporary);
        meeting_config_clear(temporary);
        if(!same)return false;
        uint32_t crc=get32(scratch+8);
        if(!io->write_sync(io->ctx,scratch,previous))return false;
        size_t checked=0;
        return io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&checked)==0&&checked==previous&&
               valid_record(scratch,checked)&&get32(scratch+8)==crc;
    }
    if(status!=1)return false;
    memcpy(scratch,"MCP1",4);put32(scratch+4,(uint32_t)length);put32(scratch+8,checksum(json,length));memcpy(scratch+12,json,length);
    if(!io->write_sync(io->ctx,scratch,length+12))return false;
    size_t checked=0;
    return io->read(io->ctx,scratch,MEETING_CONFIG_RECORD_MAX,&checked)==0&&checked==length+12&&
           valid_record(scratch,checked)&&!memcmp(scratch+12,json,length);
}
