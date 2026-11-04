#include "meeting_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool ca(const char *pem,uint8_t hash[32],void *ctx){(void)ctx;if(strcmp(pem,"dummy"))return false;memset(hash,1,32);return true;}
static const char *format="{\"version\":1,\"ssid\":\"%s\",\"password\":\"12345678\",\"origin\":\"%s\",\"deviceToken\":\"abcdefghijklmnopqrstuvwxyz012345\",\"caPem\":\"dummy\",\"caSha256\":\"%s\",\"utc\":1800000000,\"timezoneMinutes\":480%s}";
int main(void)
{
    char json[1200],hash[65];for(int i=0;i<32;i++){hash[i*2]='0';hash[i*2+1]='1';}hash[64]=0;
    meeting_config out;memset(&out,0,sizeof(out));
    int n=snprintf(json,sizeof(json),format,"test","https://example.com:443",hash,"");
    assert(n>0&&meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    assert(!strcmp(out.ssid,"test")&&!strcmp(out.origin,"https://example.com:443"));
    assert(out.utc==1800000000&&out.timezone_minutes==480&&out.ca_sha256[0]==1);
    meeting_config_clear(&out);for(size_t i=0;i<sizeof(out);i++)assert(((unsigned char *)&out)[i]==0);
    n=snprintf(json,sizeof(json),format,"test","https://example.com/path",hash,"");
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    n=snprintf(json,sizeof(json),format,"test","https://example.com:0",hash,"");
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    n=snprintf(json,sizeof(json),format,"test","https://example.com",hash,",\"ssid\":\"other\"");
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    n=snprintf(json,sizeof(json),format,"a\\u0000b","https://example.com",hash,"");
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    const char *credential="653bb1245e828fcda4fa53fcd5a3def5bd7654e651f54b4132b73d74e64435c4";
    n=snprintf(json,sizeof(json),format,"test","https://example.com",hash,"");
    char *version=strstr(json,"\"version\":1");assert(version);version[10]='2';
    assert(json[n-1]=='}');n--;n+=snprintf(json+n,sizeof(json)-(size_t)n,",\"deviceId\":\"device_1\",\"credentialId\":\"%s\"}",credential);
    assert(meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    assert(!strcmp(out.device_id,"device_1")&&!strcmp(out.credential_id,credential));
    char *id=strstr(json,"device_1");assert(id);id[0]='/';
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));id[0]='d';
    char *cred=strstr(json,credential);assert(cred);cred[0]='0';
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    hash[0]='2';n=snprintf(json,sizeof(json),format,"test","https://example.com",hash,"");
    assert(!meeting_config_parse((const uint8_t *)json,(size_t)n,ca,NULL,&out));
    return 0;
}
