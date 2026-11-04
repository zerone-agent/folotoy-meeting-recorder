#include "meeting_config.h"
#include "meeting_config_fragments.h"
#include "cJSON.h"
#include "mbedtls/sha256.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void meeting_config_clear(meeting_config *v){if(v){volatile unsigned char *p=(volatile unsigned char *)v;for(size_t i=0;i<sizeof(*v);i++)p[i]=0;}}
static const cJSON *unique(const cJSON *obj,const char *key)
{
    const cJSON *v,*result=NULL;cJSON_ArrayForEach(v,obj)if(v->string&&!strcmp(v->string,key)){if(result)return NULL;result=v;}return result;
}
static bool number(const cJSON *v,double min,double max,int64_t *out)
{
    if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<min||v->valuedouble>max||floor(v->valuedouble)!=v->valuedouble)return false;
    *out=(int64_t)v->valuedouble;return true;
}
static bool copy_string(const cJSON *v,char *dest,size_t cap,size_t min)
{
    if(!cJSON_IsString(v)||!v->valuestring)return false;
    size_t n=strlen(v->valuestring);if(n<min||n>=cap)return false;memcpy(dest,v->valuestring,n+1);return true;
}
static int digit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static bool origin_ok(const char *s)
{
    if(strncmp(s,"https://",8)||!s[8])return false;
    const char *p=s+8;bool label=false;
    for(;*p&&*p!=':';p++){
        if(*p=='.'){if(!label||p[-1]=='-')return false;label=false;}
        else if((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9'))label=true;
        else if(*p=='-'&&label){}else return false;
    }
    if(!label||p[-1]=='-')return false;
    if(*p==':'){unsigned port=0;unsigned n=0;for(p++;*p;p++){if(*p<'0'||*p>'9'||++n>5)return false;port=port*10u+(unsigned)(*p-'0');}if(!n||!port||port>65535)return false;}
    return true;
}
static bool device_id_ok(const char *s)
{
    if(!s||!*s)return false;
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='_'||*p=='-'))return false;
    return true;
}
static bool token_id_matches(const char *token,const char *credential_id)
{
    uint8_t digest[32];
    if(strlen(credential_id)!=64||mbedtls_sha256((const unsigned char *)token,strlen(token),digest,0)!=0)return false;
    for(unsigned i=0;i<32;i++){
        int h=digit(credential_id[2*i]),l=digit(credential_id[2*i+1]);
        if(h<0||l<0||digest[i]!=(uint8_t)((h<<4)|l))return false;
    }
    return true;
}
bool meeting_config_revocation_identity_valid(const meeting_config *config)
{
    if(!config||!memchr(config->device_id,0,sizeof(config->device_id))||
       !memchr(config->device_token,0,sizeof(config->device_token))||
       !memchr(config->credential_id,0,sizeof(config->credential_id))||
       strlen(config->device_token)<32||!device_id_ok(config->device_id))return false;
    for(const unsigned char *p=(const unsigned char *)config->device_token;*p;p++)
        if(*p<=32||*p>=127)return false;
    return token_id_matches(config->device_token,config->credential_id);
}
static bool no_nul(const uint8_t *s,size_t n)
{
    for(size_t i=0;i<n;i++){
        if(!s[i])return false;
        if(s[i]=='\\'&&i+5<n&&s[i+1]=='u'&&s[i+2]=='0'&&s[i+3]=='0'&&s[i+4]=='0'&&s[i+5]=='0')return false;
    }return true;
}
static void clear_json_values(cJSON *root)
{
    cJSON *item;
    cJSON_ArrayForEach(item,root){
        if(cJSON_IsString(item)&&item->valuestring){
            size_t n=strlen(item->valuestring);volatile char *p=item->valuestring;
            for(size_t i=0;i<n;i++)p[i]=0;
        }
    }
}
bool meeting_config_parse(const uint8_t *json,size_t length,meeting_ca_validate validate_ca,void *ctx,meeting_config *out)
{
    if(!json||!length||length>MEETING_CONFIG_MAX_BYTES||!validate_ca||!out||!no_nul(json,length))return false;
    const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts((const char *)json,length,&end,0);bool ok=false;
    meeting_config *candidate=calloc(1,sizeof(*candidate));int64_t n;uint8_t actual[32];
    if(!root||!end||!candidate)goto done;
    while(end<(const char *)json+length&&(*end==' '||*end=='\n'||*end=='\r'||*end=='\t'))end++;
    if(end!=(const char *)json+length||!cJSON_IsObject(root))goto done;
    int64_t version=0;
    if(!number(unique(root,"version"),1,2,&version)||cJSON_GetArraySize(root)!=(version==1?9:11))goto done;
    if(
       !copy_string(unique(root,"ssid"),candidate->ssid,sizeof(candidate->ssid),1)||
       !copy_string(unique(root,"password"),candidate->password,sizeof(candidate->password),0)||
       !copy_string(unique(root,"origin"),candidate->origin,sizeof(candidate->origin),9)||
       !copy_string(unique(root,"deviceToken"),candidate->device_token,sizeof(candidate->device_token),32)||
       !copy_string(unique(root,"caPem"),candidate->ca_pem,sizeof(candidate->ca_pem),1))goto done;
    size_t password_len=strlen(candidate->password);if(password_len&&password_len<8)goto done;
    if(!origin_ok(candidate->origin))goto done;
    for(const unsigned char *p=(const unsigned char *)candidate->device_token;*p;p++)if(*p<=32||*p>=127)goto done;
    if(version==2&&(!copy_string(unique(root,"deviceId"),candidate->device_id,sizeof(candidate->device_id),1)||
       !copy_string(unique(root,"credentialId"),candidate->credential_id,sizeof(candidate->credential_id),64)||
       !meeting_config_revocation_identity_valid(candidate)))goto done;
    const cJSON *fingerprint=unique(root,"caSha256");
    if(!cJSON_IsString(fingerprint)||!fingerprint->valuestring||strlen(fingerprint->valuestring)!=64)goto done;
    for(unsigned i=0;i<32;i++){int a=digit(fingerprint->valuestring[i*2]),b=digit(fingerprint->valuestring[i*2+1]);if(a<0||b<0)goto done;candidate->ca_sha256[i]=(uint8_t)((a<<4)|b);}
    if(!validate_ca(candidate->ca_pem,actual,ctx)||memcmp(actual,candidate->ca_sha256,32))goto done;
    if(!number(unique(root,"utc"),1735689600.0,4102444800.0,&candidate->utc)||
       !number(unique(root,"timezoneMinutes"),-720,840,&n))goto done;
    candidate->timezone_minutes=(int16_t)n;*out=*candidate;ok=true;
done:
    meeting_config_clear(candidate);free(candidate);if(root)clear_json_values(root);cJSON_Delete(root);return ok;
}
