#include "meeting_time_client.h"
#include "mbedtls/sha256.h"
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
typedef struct {int port;int64_t floor;int writes,sets;} fixture;
static bool rng(void *ctx,uint8_t nonce[32]){(void)ctx;memset(nonce,7,32);return true;}
static bool uptime(void *ctx,uint64_t *ms){(void)ctx;*ms=1000;return true;}
static bool cancelled(void *ctx){(void)ctx;return false;}
static int read_floor(void *ctx,int64_t *floor){*floor=((fixture *)ctx)->floor;return 0;}
static bool commit_floor(void *ctx,int64_t floor){fixture *f=ctx;f->floor=floor;f->writes++;return true;}
static bool set_wall(void *ctx,int64_t epoch){fixture *f=ctx;assert(epoch==1800000000);f->sets++;return true;}
static bool fetch(void *ctx,const uint8_t nonce[32],char *out,size_t cap,size_t *len,int *status){
 fixture *f=ctx;int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return false;
 struct sockaddr_in addr={.sin_family=AF_INET,.sin_port=htons((uint16_t)f->port),.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
 if(connect(fd,(struct sockaddr *)&addr,sizeof addr)!=0){close(fd);return false;}
 char hex[65];for(size_t i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",nonce[i]);
 char request[256];int n=snprintf(request,sizeof request,"GET /meeting-service/time-proof/%s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n",hex);
 if(n<=0||write(fd,request,(size_t)n)!=n){close(fd);return false;}
 char response[2048];size_t used=0;for(;;){if(used==sizeof(response)-1){close(fd);return false;}ssize_t got=read(fd,response+used,sizeof(response)-1-used);if(got<0){close(fd);return false;}if(!got)break;used+=(size_t)got;}close(fd);response[used]=0;
 char *body=strstr(response,"\r\n\r\n");if(!body||sscanf(response,"HTTP/%*u.%*u %d",status)!=1)return false;body+=4;size_t count=used-(size_t)(body-response);if(count>cap)return false;memcpy(out,body,count);*len=count;return true;
}
int main(int argc,char **argv){
 assert(argc==3);FILE *in=fopen(argv[1],"rb");assert(in);meeting_time_anchor anchor={0};assert(fread(anchor.key.public_key,1,65,in)==65);assert(fgetc(in)==EOF);fclose(in);
 const char *origin="https://127.0.0.1";assert(mbedtls_sha256((const unsigned char*)origin,strlen(origin),anchor.origin_sha256,0)==0);
 fixture f={.port=atoi(argv[2]),.floor=1799999999};assert(f.port>0&&f.port<=65535);
 meeting_time_client_io client={&f,rng,uptime,cancelled,fetch};meeting_clock_io io={&f,read_floor,commit_floor,set_wall};meeting_time_challenge challenge={0};meeting_clock clock={0};
 assert(meeting_time_client_run(&challenge,origin,&anchor,&client,&io,&clock)==MEETING_TIME_CLIENT_OK);
 assert(clock.trusted&&f.floor==1800000000&&f.writes==1&&f.sets==1);return 0;
}
