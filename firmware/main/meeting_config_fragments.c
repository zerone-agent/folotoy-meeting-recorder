#include "meeting_config_fragments.h"
#include <string.h>
void meeting_config_fragments_reset(meeting_config_fragments *s)
{
    if(!s)return;
    volatile uint8_t *p=s->bytes;for(size_t i=0;i<sizeof(s->bytes);i++)p[i]=0;
    s->session=0;s->expected=0;s->received=0;s->active=false;s->sealed=false;
}
bool meeting_config_fragments_begin(meeting_config_fragments *s,uint32_t session,uint16_t length)
{
    if(!s)return false;
    if(s->active||s->sealed||!length||length>MEETING_CONFIG_MAX_BYTES){meeting_config_fragments_reset(s);return false;}
    s->session=session;s->expected=length;s->received=0;s->active=true;return true;
}
bool meeting_config_fragments_append(meeting_config_fragments *s,uint32_t session,uint16_t offset,
                                     const uint8_t *bytes,size_t length)
{
    if(!s)return false;
    if(!s->active||s->sealed||session!=s->session||!bytes||!length||offset!=s->received||
       length>(size_t)(s->expected-s->received)){meeting_config_fragments_reset(s);return false;}
    memcpy(s->bytes+s->received,bytes,length);s->received+=(uint16_t)length;return true;
}
bool meeting_config_fragments_seal(meeting_config_fragments *s,uint32_t session,const uint8_t **bytes,size_t *length)
{
    if(!s)return false;
    if(!bytes||!length||!s->active||s->sealed||session!=s->session||s->received!=s->expected){
        meeting_config_fragments_reset(s);return false;
    }
    s->active=false;s->sealed=true;*bytes=s->bytes;*length=s->received;return true;
}
