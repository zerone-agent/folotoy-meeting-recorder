#include "meeting_agent_anchor.h"
#include <string.h>
static int nibble(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static bool decode(const char *text,uint8_t *out,size_t n){
 if(!text||strlen(text)!=2*n)return false;
 for(size_t i=0;i<n;i++){int a=nibble(text[i*2]),b=nibble(text[i*2+1]);if(a<0||b<0)return false;out[i]=(uint8_t)((a<<4)|b);}return true;
}
typedef struct {bool allowed;const meeting_time_anchor *expected;} authorization;
static bool authorized(void *ctx,const meeting_time_anchor *anchor){
 authorization *a=ctx;return a->allowed&&!memcmp(anchor->origin_sha256,a->expected->origin_sha256,32)&&
    !memcmp(anchor->key.public_key,a->expected->key.public_key,65);
}
bool meeting_agent_anchor_matches(const meeting_time_anchor *anchor,const uint8_t configured_origin[32],
 const char *expected_origin_hex,const char *public_key_hex,bool device_allowed){
 meeting_time_anchor expected;
 return anchor&&configured_origin&&device_allowed&&decode(expected_origin_hex,expected.origin_sha256,32)&&
    decode(public_key_hex,expected.key.public_key,65)&&!memcmp(configured_origin,expected.origin_sha256,32)&&
    !memcmp(anchor->origin_sha256,expected.origin_sha256,32)&&!memcmp(anchor->key.public_key,expected.key.public_key,65);
}
meeting_time_anchor_result meeting_agent_anchor_install(meeting_time_anchor_session *session,
 const meeting_time_anchor_io *io,const uint8_t configured_origin[32],const char *expected_origin_hex,
 const char *public_key_hex,bool device_allowed,meeting_time_key_check check,void *key_ctx){
 meeting_time_anchor value;
 if(!configured_origin||!device_allowed||!decode(expected_origin_hex,value.origin_sha256,32)||
    !decode(public_key_hex,value.key.public_key,65)||memcmp(configured_origin,value.origin_sha256,32))return MEETING_ANCHOR_DENIED;
 authorization auth={device_allowed,&value};
 return meeting_time_anchor_install(session,io,&value,authorized,&auth,check,key_ctx);
}
