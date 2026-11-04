#include "meeting_cancel_request.h"
#include <stdio.h>
#include <string.h>

int meeting_cancel_request(const uint8_t meeting_id[16],const meeting_manifest *expected,const meeting_upload_transport *transport)
{
    if(!meeting_id||!transport||!transport->request||!transport->cancelled||transport->cancelled(transport->ctx))return 0;
    static const char hex[]="0123456789abcdef";char uuid[37];unsigned p=0,nonzero=0;
    for(unsigned i=0;i<16;i++){
        if(i==4||i==6||i==8||i==10)uuid[p++]='-';
        uuid[p++]=hex[meeting_id[i]>>4];uuid[p++]=hex[meeting_id[i]&15];nonzero|=meeting_id[i];
    }
    uuid[p]='\0';if(!nonzero||(expected&&memcmp(expected->id,uuid,sizeof(uuid))))return 0;
    char path[80],response[4097];int status;size_t length=0;
    int n=snprintf(path,sizeof(path),"/meeting-service/meetings/%s/cancel",uuid);
    if(n<0||(size_t)n>=sizeof(path)||!transport->request(transport->ctx,"POST",path,"application/json",NULL,NULL,0,
       &status,response,sizeof(response)-1,&length)||length>sizeof(response)-1||transport->cancelled(transport->ctx))return 0;
    return (int)meeting_cancel_json_parse(response,length,status,expected);
}
