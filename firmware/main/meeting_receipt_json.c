#include "meeting_receipt_json.h"
#include "cJSON.h"
#include <string.h>
#include <math.h>

static const cJSON *unique(const cJSON *o, const char *key)
{
    const cJSON *x, *found=NULL;
    cJSON_ArrayForEach(x,o) if (x->string && strcmp(x->string,key)==0) { if(found)return NULL; found=x; }
    return found;
}
static bool number(const cJSON *v,double min,double max,uint64_t *out)
{
    if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<min||v->valuedouble>max||floor(v->valuedouble)!=v->valuedouble)return false;
    *out=(uint64_t)v->valuedouble;return true;
}
static int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static bool no_nul_escape(const char *s,size_t n)
{
    for(size_t i=0;i<n;i++) {
        if(s[i]=='\0')return false;
        if(s[i]=='\\' && i+5<n && s[i+1]=='u' && s[i+2]=='0' && s[i+3]=='0' && s[i+4]=='0' && s[i+5]=='0')return false;
    }
    return true;
}
bool meeting_receipt_json_matches(const char *json,size_t length,int status,const meeting_manifest *expected)
{
    if(!json||!expected||status!=200||length==0||length>4096||!no_nul_escape(json,length))return false;
    const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(json,length,&end,0);bool ok=false;
    if(!root||!end)goto done;
    while(end<json+length&&(*end==' '||*end=='\t'||*end=='\r'||*end=='\n'))end++;
    if(end!=json+length||!cJSON_IsObject(root))goto done;
    const cJSON *id=unique(root,"id"),*committed=unique(root,"committed"),*m=unique(root,"manifest");
    if(!cJSON_IsString(id)||strlen(id->valuestring)!=36||!cJSON_IsTrue(committed)||!cJSON_IsObject(m))goto done;
    const cJSON *count=unique(m,"count"),*bytes=unique(m,"bytes"),*digest=unique(m,"digest"),*reason=unique(m,"reason"),*time=unique(m,"recordedAt"),*trust=unique(m,"timeTrust");
    uint64_t n;meeting_manifest v={0};memcpy(v.id,id->valuestring,37);
    if(!number(count,1,8192,&n))goto done;
    v.count=(uint32_t)n;
    if(!number(bytes,1,20000000,&n))goto done;
    v.bytes=n;
    if(!cJSON_IsString(digest)||strlen(digest->valuestring)!=64)goto done;
    for(unsigned i=0;i<32;i++){int h=nibble(digest->valuestring[2*i]),l=nibble(digest->valuestring[2*i+1]);if(h<0||l<0)goto done;v.digest[i]=(uint8_t)((h<<4)|l);}
    if(!cJSON_IsString(reason))goto done;
    const char *reasons[]={"user","limit","storage","interrupted"};unsigned i;for(i=0;i<4;i++)if(!strcmp(reason->valuestring,reasons[i]))break;if(i==4)goto done;v.reason=(uint8_t)i;
    if(cJSON_IsNull(time))v.recorded_at_present=false;else{if(!number(time,946684800,4102444800.0,&n))goto done;v.recorded_at_present=true;v.recorded_at=(int64_t)n;}
    if(!cJSON_IsString(trust))goto done;
    const char *trusts[]={"uncalibrated","unverified","trusted"};for(i=0;i<3;i++)if(!strcmp(trust->valuestring,trusts[i]))break;if(i==3)goto done;v.time_trust=(uint8_t)i;
    ok=meeting_receipt_matches(expected,&v,true);
done: cJSON_Delete(root);return ok;
}

static cJSON *bounded_object(const char *json,size_t length)
{
    if(!json||!length||length>4096||!no_nul_escape(json,length))return NULL;
    const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(json,length,&end,0);
    if(!root||!end){cJSON_Delete(root);return NULL;}
    while(end<json+length&&(*end==' '||*end=='\t'||*end=='\r'||*end=='\n'))end++;
    if(end!=json+length||!cJSON_IsObject(root)){cJSON_Delete(root);return NULL;}
    return root;
}
meeting_cancel_response meeting_cancel_json_parse(const char *json,size_t length,int status,const meeting_manifest *expected)
{
    if(status!=200)return MEETING_CANCEL_UNKNOWN;
    cJSON *root=bounded_object(json,length);if(!root)return MEETING_CANCEL_UNKNOWN;
    const cJSON *state=unique(root,"state");meeting_cancel_response result=MEETING_CANCEL_UNKNOWN;
    if(cJSON_IsString(state)&&!strcmp(state->valuestring,"cancelled")){
        /* Cancelled responses have only a state; contradictory receipts are
         * rejected rather than interpreted as successful cancellation. */
        if(!cJSON_GetObjectItemCaseSensitive(root,"id")&&!cJSON_GetObjectItemCaseSensitive(root,"committed")&&
           !cJSON_GetObjectItemCaseSensitive(root,"manifest"))result=MEETING_CANCEL_CANCELLED;
    }else if(cJSON_IsString(state)&&!strcmp(state->valuestring,"committed")&&expected&&
             meeting_receipt_json_matches(json,length,status,expected))result=MEETING_CANCEL_COMMITTED;
    cJSON_Delete(root);return result;
}
bool meeting_receipt_json_integrity_error(const char *json,size_t length,int status)
{
    if(status!=409)return false;
    cJSON *root=bounded_object(json,length);if(!root)return false;
    const cJSON *error=unique(root,"error");
    bool ok=cJSON_IsString(error)&&!strcmp(error->valuestring,"integrity");
    cJSON_Delete(root);return ok;
}
bool meeting_repair_json_parse(const char *json,size_t length,int status,const meeting_manifest *expected,
                               uint8_t *bitmap,size_t bitmap_size)
{
    if(status!=200||!expected||!bitmap||!expected->count||expected->count>8192||
       bitmap_size<(expected->count+7)/8)return false;
    memset(bitmap,0,bitmap_size);
    cJSON *root=bounded_object(json,length);if(!root)return false;
    const cJSON *id=unique(root,"id"),*overall=unique(root,"overall"),*count=unique(root,"expectedCount"),
        *missing=unique(root,"missingCount"),*damaged=unique(root,"damagedSequences");
    bool ok=false;uint64_t value;
    if(!cJSON_IsString(id)||strcmp(id->valuestring,expected->id)||!cJSON_IsString(overall)||
       !cJSON_IsArray(damaged))goto done;
    if(!strcmp(overall->valuestring,"failed")){
        if(!number(count,1,8192,&value)||value!=expected->count||!number(missing,0,8192,&value)||value!=0)goto done;
    }else if(strcmp(overall->valuestring,"not_committed")||!cJSON_IsNull(count)||!cJSON_IsNull(missing))goto done;
    const cJSON *item;size_t found=0;
    cJSON_ArrayForEach(item,damaged){
        if(!number(item,0,expected->count-1,&value)||(bitmap[value/8]&(1u<<(value%8))))goto done;
        bitmap[value/8]|=(uint8_t)(1u<<(value%8));found++;
    }
    ok=found>0;
done:
    if(!ok)memset(bitmap,0,bitmap_size);
    cJSON_Delete(root);return ok;
}
bool meeting_chunk_window_json_parse(const char *json,size_t length,int status,const char *id,
                                     uint32_t first,meeting_remote_chunk entries[16])
{
    if(!id||!entries||status!=200||first>8191||first%16)return false;
    memset(entries,0,16*sizeof(*entries));cJSON *root=bounded_object(json,length);if(!root)return false;
    const cJSON *rid=unique(root,"id"),*start=unique(root,"first"),*chunks=unique(root,"chunks");
    uint64_t n;bool ok=false;
    if(!cJSON_IsString(rid)||strcmp(rid->valuestring,id)||!number(start,0,8191,&n)||n!=first||!cJSON_IsArray(chunks))goto done;
    const cJSON *row;
    cJSON_ArrayForEach(row,chunks){
        if(!cJSON_IsObject(row))goto done;
        const cJSON *seq=unique(row,"seq"),*size=unique(row,"size"),*digest=unique(row,"digest"),*valid=unique(row,"valid");
        if(!number(seq,first,first+15,&n))goto done;
        meeting_remote_chunk *entry=&entries[n-first];if(entry->present)goto done;
        if(!number(size,1,262144,&n)||!cJSON_IsString(digest)||strlen(digest->valuestring)!=64||!cJSON_IsBool(valid))goto done;
        entry->size=(uint32_t)n;entry->present=true;entry->valid=cJSON_IsTrue(valid);
        for(unsigned i=0;i<32;i++){int h=nibble(digest->valuestring[i*2]),l=nibble(digest->valuestring[i*2+1]);if(h<0||l<0)goto done;entry->digest[i]=(uint8_t)((h<<4)|l);}
    }
    ok=true;
done:
    if(!ok)memset(entries,0,16*sizeof(*entries));
    cJSON_Delete(root);return ok;
}

bool meeting_legacy_chunk_window_json_parse(const char *json,size_t length,int status,const char *id,
                                            uint32_t first,meeting_remote_chunk entries[16])
{
    if(!id||!entries||status!=200||first>8191||first%16)return false;
    memset(entries,0,16*sizeof(*entries));cJSON *root=bounded_object(json,length);if(!root)return false;
    const cJSON *rid=unique(root,"id"),*chunks=unique(root,"chunks");bool ok=false;uint64_t n;
    if(!cJSON_IsString(rid)||strcmp(rid->valuestring,id)||!cJSON_IsArray(chunks))goto done;
    const cJSON *row;
    cJSON_ArrayForEach(row,chunks){
        if(!cJSON_IsObject(row)||!number(unique(row,"seq"),0,8191,&n))goto done;
        if(n<first||n>=first+16)continue;
        meeting_remote_chunk *entry=&entries[n-first];if(entry->present)goto done;
        const cJSON *size=unique(row,"size"),*digest=unique(row,"digest"),*valid=unique(row,"valid");
        if(!number(size,1,262144,&n)||!cJSON_IsString(digest)||strlen(digest->valuestring)!=64||!cJSON_IsBool(valid))goto done;
        entry->size=(uint32_t)n;entry->present=true;entry->valid=cJSON_IsTrue(valid);
        for(unsigned i=0;i<32;i++){int h=nibble(digest->valuestring[2*i]),l=nibble(digest->valuestring[2*i+1]);if(h<0||l<0)goto done;entry->digest[i]=(uint8_t)((h<<4)|l);}
    }
    ok=true;
done:
    if(!ok)memset(entries,0,16*sizeof(*entries));
    cJSON_Delete(root);return ok;
}
