#include "meeting_time_http.h"
#include "meeting_dns.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define TIME_HTTP_BUDGET_US 10000000LL
static bool origin_host(const char *origin,const char *scheme,const char **host,size_t *length){
 size_t prefix=strlen(scheme),total=0;if(!origin)return false;
 while(total<=200&&origin[total])total++;
 if(total>200||total<prefix||strncmp(origin,scheme,prefix))return false;
 const char *p=origin+prefix,*start=p;bool label=false;if(!*p)return false;
 while(*p&&*p!=':'){
  if(*p=='.'){if(!label||p[-1]=='-')return false;label=false;}
  else if((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9'))label=true;
  else if(*p=='-'&&label){}else return false;
  p++;
 }
 if(!label||p[-1]=='-'||(size_t)(p-start)>180)return false;
 *host=start;*length=(size_t)(p-start);
 if(*p==':'){unsigned port=0,digits=0;for(p++;*p;p++){if(*p<'0'||*p>'9'||++digits>5)return false;port=port*10u+(unsigned)(*p-'0');}if(!digits||!port||port>65535)return false;}
 return true;
}
static char lower_ascii(char c){return c>='A'&&c<='Z'?(char)(c-'A'+'a'):c;}
static bool same_host(const char *https,const char *http){
 const char *a,*b;size_t an,bn;
 if(!origin_host(https,"https://",&a,&an)||!origin_host(http,"http://",&b,&bn)||an!=bn)return false;
 for(size_t i=0;i<an;i++)if(lower_ascii(a[i])!=lower_ascii(b[i]))return false;
 return true;
}
bool meeting_time_http_fetch(void *ctx,const uint8_t nonce[32],char *response,size_t capacity,size_t *length,int *status){
 meeting_time_http *c=ctx;if(!c||!c->cancelled||!nonce||!response||capacity<2||capacity>INT_MAX||!length||!status||c->cancelled(c->cancel_ctx)||!same_host(c->https_origin,c->http_origin))return false;
 static const char hex[]="0123456789abcdef";char n[65];uint8_t any=0;for(unsigned i=0;i<32;i++){any|=nonce[i];n[2*i]=hex[nonce[i]>>4];n[2*i+1]=hex[nonce[i]&15];}n[64]=0;if(!any)return false;
 int64_t start=esp_timer_get_time();if(start<0||start>INT64_MAX-TIME_HTTP_BUDGET_US)return false;int64_t deadline=start+TIME_HTTP_BUDGET_US;
 const char *host;size_t host_len;if(!origin_host(c->http_origin,"http://",&host,&host_len))return false;
 char hostname[181],ip[16];memcpy(hostname,host,host_len);hostname[host_len]=0;
 if(!meeting_dns_resolve(hostname,c->cancelled,c->cancel_ctx,deadline,ip))return false;
 char url[300];int written=snprintf(url,sizeof url,"http://%s%s/meeting-service/time-proof/%s",ip,host+host_len,n);if(written<0||(size_t)written>=sizeof url)return false;
 esp_http_client_config_t cfg={.url=url,.timeout_ms=3000,.disable_auto_redirect=true};
 esp_http_client_handle_t client=esp_http_client_init(&cfg);if(!client)return false;bool ok=false;size_t total=0;
 if(esp_http_client_set_method(client,HTTP_METHOD_GET)!=ESP_OK||esp_http_client_set_header(client,"Host",host)!=ESP_OK||esp_http_client_open(client,0)!=ESP_OK)goto done;
 for(;;){
  if(c->cancelled(c->cancel_ctx)||esp_timer_get_time()>=deadline)goto done;
  int64_t headers=esp_http_client_fetch_headers(client);if(headers>=0)break;if(headers!=-ESP_ERR_HTTP_EAGAIN)goto done;
 }
 *status=esp_http_client_get_status_code(client);if(*status!=200)goto done;
 for(;;){
  if(c->cancelled(c->cancel_ctx)||esp_timer_get_time()>=deadline||total>=capacity)goto done;
  int nread=esp_http_client_read(client,response+total,(int)(capacity-total));
  if(nread==-ESP_ERR_HTTP_EAGAIN)continue;
  if(nread<0)goto done;
  if(nread==0){if(!esp_http_client_is_complete_data_received(client))goto done;break;}
  total+=(size_t)nread;
 }
 if(c->cancelled(c->cancel_ctx)||esp_timer_get_time()>=deadline)goto done;
 *length=total;ok=true;
done:(void)esp_http_client_cleanup(client);return ok;
}
