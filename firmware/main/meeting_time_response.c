#include "meeting_time_response.h"
#include "cJSON.h"
#include <string.h>
#define MAX_RESPONSE 512u
static int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static bool decode_hex(const cJSON *item,uint8_t *out,size_t n){
 if(!cJSON_IsString(item)||!item->valuestring||strlen(item->valuestring)!=n*2)return false;
 for(size_t i=0;i<n;i++){int a=nibble(item->valuestring[2*i]),b=nibble(item->valuestring[2*i+1]);if(a<0||b<0)return false;out[i]=(uint8_t)((a<<4)|b);}return true;
}
static const cJSON *unique(const cJSON *root,const char *name){
 const cJSON *entry,*found=NULL;cJSON_ArrayForEach(entry,root)if(entry->string&&!strcmp(entry->string,name)){if(found)return NULL;found=entry;}return found;
}
static bool safe_text(const char *json,size_t n){
 for(size_t i=0;i<n;i++)if(!json[i]||(json[i]=='\\'&&i+5<n&&json[i+1]=='u'&&json[i+2]=='0'&&json[i+3]=='0'&&json[i+4]=='0'&&json[i+5]=='0'))return false;
 return true;
}
bool meeting_time_response_parse(const char *json,size_t length,int status,const uint8_t expected_key_id[32],meeting_time_response *out){
 if(!json||!expected_key_id||!out||status!=200||!length||length>MAX_RESPONSE||!safe_text(json,length))return false;
 const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(json,length,&end,0);bool ok=false;meeting_time_response value={0};uint8_t key_id[32];
 if(!root||!end)goto done;
 while(end<json+length&&(*end==' '||*end=='\r'||*end=='\n'||*end=='\t'))end++;
 if(end!=json+length||!cJSON_IsObject(root)||cJSON_GetArraySize(root)!=3)goto done;
 const cJSON *key=unique(root,"keyId"),*message=unique(root,"message"),*signature=unique(root,"signature");
 if(!decode_hex(key,key_id,sizeof key_id)||memcmp(key_id,expected_key_id,sizeof key_id)||!decode_hex(message,value.message,sizeof value.message)||
    !cJSON_IsString(signature)||!signature->valuestring)goto done;
 size_t hexlen=strlen(signature->valuestring);
 if(hexlen<16||hexlen>MEETING_TIME_PROOF_MAX_SIGNATURE*2||hexlen%2)goto done;
 value.signature_len=hexlen/2;
 if(!decode_hex(signature,value.signature,value.signature_len))goto done;
 *out=value;ok=true;
done:cJSON_Delete(root);return ok;
}
