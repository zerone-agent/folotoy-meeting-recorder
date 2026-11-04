#include "meeting_time_response.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 char key[65],message[153],sig[17],json[512];uint8_t expected[32];memset(expected,0x11,sizeof expected);
 for(size_t i=0;i<64;i++)key[i]='1';key[64]=0;for(size_t i=0;i<152;i++)message[i]='2';message[152]=0;memset(sig,'3',16);sig[16]=0;
 int n=snprintf(json,sizeof json,"{\"keyId\":\"%s\",\"message\":\"%s\",\"signature\":\"%s\"}",key,message,sig);assert(n>0&&(size_t)n<sizeof json);
 meeting_time_response out;memset(&out,0x5a,sizeof out);
 assert(meeting_time_response_parse(json,(size_t)n,200,expected,&out)&&out.signature_len==8&&out.message[0]==0x22&&out.signature[0]==0x33);
 assert(!meeting_time_response_parse(json,(size_t)n,503,expected,&out));
 expected[0]^=1;assert(!meeting_time_response_parse(json,(size_t)n,200,expected,&out));expected[0]^=1;
 char *signature=strstr(json,"3333333333333333");assert(signature);signature[0]='A';assert(!meeting_time_response_parse(json,(size_t)n,200,expected,&out));signature[0]='3';
 char duplicate[512];int d=snprintf(duplicate,sizeof duplicate,"{\"keyId\":\"%s\",\"message\":\"%s\",\"signature\":\"%s\",\"signature\":\"%s\"}",key,message,sig,sig);assert(d>0);
 assert(!meeting_time_response_parse(duplicate,(size_t)d,200,expected,&out));
 assert(!meeting_time_response_parse(json,(size_t)n-1,200,expected,&out));
 json[n-1]=0;assert(!meeting_time_response_parse(json,(size_t)n,200,expected,&out));
 return 0;
}
