#include "meeting_config_protocol.h"
bool meeting_config_protocol_request(meeting_config_fragments *s,uint32_t session,const uint8_t *request,size_t length)
{
    if(!s)return false;
    if(!request||!length){meeting_config_fragments_reset(s);return false;}
    switch(request[0]){
    case 1:
        if(length==3)return meeting_config_fragments_begin(s,session,(uint16_t)(request[1]|((uint16_t)request[2]<<8)));
        break;
    case 2:
        if(length>3)return meeting_config_fragments_append(s,session,(uint16_t)(request[1]|((uint16_t)request[2]<<8)),request+3,length-3);
        break;
    case 3: {
        const uint8_t *data;size_t size;
        if(length==1)return meeting_config_fragments_seal(s,session,&data,&size);
        break;
    }
    default: break;
    }
    meeting_config_fragments_reset(s);return false;
}
