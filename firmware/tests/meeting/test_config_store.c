#include "meeting_config_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const uint8_t json[]="{\"version\":1,\"ssid\":\"wifi\",\"password\":\"12345678\",\"origin\":\"https://example.com\",\"deviceToken\":\"abcdefghijklmnopqrstuvwxyz012345\",\"caPem\":\"dummy\",\"caSha256\":\"0101010101010101010101010101010101010101010101010101010101010101\",\"utc\":1800000000,\"timezoneMinutes\":480}";
typedef struct {uint8_t bytes[MEETING_CONFIG_RECORD_MAX];size_t length;int fail_read,fail_write,corrupt,writes;} fixture;
static int read_data(void *ctx,uint8_t *out,size_t capacity,size_t *length){fixture *f=ctx;if(f->fail_read)return -1;if(!f->length)return 1;assert(capacity>=f->length);memcpy(out,f->bytes,f->length);*length=f->length;return 0;}
static bool write_data(void *ctx,const uint8_t *data,size_t length){fixture *f=ctx;f->writes++;memcpy(f->bytes,data,length);f->length=length;if(f->corrupt)f->bytes[length-1]^=1;return !f->fail_write;}
static bool ca(const char *pem,uint8_t hash[32],void *ctx){(void)ctx;if(strcmp(pem,"dummy")&&strcmp(pem,"other"))return false;memset(hash,1,32);return true;}
int main(void)
{
    fixture f={0};meeting_config_store_io io={&f,read_data,write_data};uint8_t scratch[MEETING_CONFIG_RECORD_MAX];meeting_config config;
    assert(meeting_config_store_load(&io,ca,NULL,scratch,&config)==MEETING_CONFIG_ABSENT);
    assert(!meeting_config_store_commit(&io,json,sizeof(json)-1,false,true,ca,NULL,scratch,&config)&&!f.length);
    assert(!meeting_config_store_commit(&io,json,sizeof(json)-1,true,false,ca,NULL,scratch,&config)&&!f.length);
    f.fail_write=1;assert(!meeting_config_store_commit(&io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
    f.fail_write=0;assert(meeting_config_store_commit(&io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
    assert(meeting_config_store_load(&io,ca,NULL,scratch,&config)==MEETING_CONFIG_STORED&&!strcmp(config.ssid,"wifi"));
    uint8_t fresh[sizeof(json)];memcpy(fresh,json,sizeof(json));char *utc=strstr((char *)fresh,"1800000000");assert(utc);memcpy(utc,"1800000100",10);
    assert(meeting_config_store_commit(&io,fresh,sizeof(fresh)-1,true,true,ca,NULL,scratch,&config));
    assert(meeting_config_store_load(&io,ca,NULL,scratch,&config)==MEETING_CONFIG_STORED&&config.utc==1800000000);
    uint8_t changed[sizeof(json)];memcpy(changed,json,sizeof(json));char *ssid=strstr((char *)changed,"wifi");assert(ssid);memcpy(ssid,"test",4);
    assert(!meeting_config_store_commit(&io,changed,sizeof(changed)-1,true,true,ca,NULL,scratch,&config));
    assert(meeting_config_store_load(&io,ca,NULL,scratch,&config)==MEETING_CONFIG_STORED&&!strcmp(config.ssid,"wifi"));
    /* Version 2 identity cannot silently replace an existing version 1
     * configuration, even when all existing settings are unchanged. */
    char v2[sizeof(json)+220];memcpy(v2,json,sizeof(json));char *version=strstr(v2,"\"version\":1");assert(version);version[10]='2';
    size_t used=strlen(v2);assert(v2[used-1]=='}');
    int added=snprintf(v2+used-1,sizeof(v2)-used+1,",\"deviceId\":\"dev_1\",\"credentialId\":\"653bb1245e828fcda4fa53fcd5a3def5bd7654e651f54b4132b73d74e64435c4\"}");
    assert(added>0&&(size_t)added<sizeof(v2)-used+1);
    uint8_t before_v2[MEETING_CONFIG_RECORD_MAX];size_t before_v2_len=f.length;memcpy(before_v2,f.bytes,f.length);int before_v2_writes=f.writes;
    assert(!meeting_config_store_commit(&io,(const uint8_t *)v2,strlen(v2),true,true,ca,NULL,scratch,&config));
    assert(f.writes==before_v2_writes&&f.length==before_v2_len&&!memcmp(f.bytes,before_v2,before_v2_len));
    /* A newly provisioned trust anchor must never replace the committed one
     * through the retry path, even if the candidate otherwise parses. */
    uint8_t ca_changed[sizeof(json)];memcpy(ca_changed,json,sizeof(json));
    char *pem=strstr((char *)ca_changed,"dummy");assert(pem);memcpy(pem,"other",5);
    uint8_t original[MEETING_CONFIG_RECORD_MAX];size_t original_length=f.length;
    memcpy(original,f.bytes,original_length);int original_writes=f.writes;
    assert(!meeting_config_store_commit(&io,ca_changed,sizeof(ca_changed)-1,true,true,ca,NULL,scratch,&config));
    assert(f.writes==original_writes&&f.length==original_length&&!memcmp(f.bytes,original,original_length));
    meeting_config expected,temporary;
    assert(meeting_config_parse(ca_changed,sizeof(ca_changed)-1,ca,NULL,&expected));
    assert(!meeting_config_store_commit_preparsed(&io,ca_changed,sizeof(ca_changed)-1,ca,NULL,scratch,&expected,&temporary));
    assert(f.writes==original_writes&&f.length==original_length&&!memcmp(f.bytes,original,original_length));
    f.bytes[f.length-1]^=1;assert(meeting_config_store_load(&io,ca,NULL,scratch,&config)==MEETING_CONFIG_INVALID&&config.ssid[0]==0);
    assert(!meeting_config_store_commit(&io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
    f.length=0;f.corrupt=1;assert(!meeting_config_store_commit(&io,json,sizeof(json)-1,true,true,ca,NULL,scratch,&config));
    return 0;
}
