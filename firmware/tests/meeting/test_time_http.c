#include "meeting_time_http.h"
#include "esp_http_client.h"
#include <assert.h>
#include <string.h>
static int cancelled_flag,initializations,cleanups,complete,headers_wait,read_wait,redirect_status=200,cancel_on_wait;
static int64_t now_us;
bool meeting_dns_resolve(const char *host,bool (*cancel)(void *),void *ctx,int64_t deadline,char ip[16]){assert(!strcmp(host,"example.com")&&deadline>now_us&&!cancel(ctx));strcpy(ip,"192.0.2.1");return true;}
int64_t esp_timer_get_time(void){return now_us;}
static bool cancelled(void *ctx){(void)ctx;return cancelled_flag;}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c){assert(!c->cert_pem&&c->disable_auto_redirect&&c->timeout_ms==3000);assert(!strncmp(c->url,"http://192.0.2.1:8080/meeting-service/time-proof/",49));initializations++;return (void*)1;}
int esp_http_client_set_header(void *h,const char *name,const char *value){assert(h&&!strcmp(name,"Host")&&!strcmp(value,"example.com:8080"));return ESP_OK;}
int esp_http_client_set_method(void *h,esp_http_client_method_t m){assert(h&&m==HTTP_METHOD_GET);return ESP_OK;}
int esp_http_client_open(void *h,int n){assert(h&&n==0);complete=0;return ESP_OK;}
int64_t esp_http_client_fetch_headers(void *h){assert(h);if(headers_wait){headers_wait--;now_us+=3000000;if(cancel_on_wait)cancelled_flag=1;return -ESP_ERR_HTTP_EAGAIN;}return 2;}
int esp_http_client_get_status_code(void *h){assert(h);return redirect_status;}
int esp_http_client_read(void *h,char *out,int n){assert(h);if(read_wait){read_wait--;now_us+=3000000;return -ESP_ERR_HTTP_EAGAIN;}if(complete)return 0;assert(n>=2);memcpy(out,"{}",2);complete=1;return 2;}
bool esp_http_client_is_complete_data_received(void *h){assert(h);return complete;}
int esp_http_client_cleanup(void *h){assert(h);cleanups++;return ESP_OK;}
int main(void){
 meeting_time_http c={.https_origin="https://example.com:443",.http_origin="http://example.com:8080",.cancelled=cancelled};uint8_t nonce[32]={1};char response[20];size_t n=0;int status=0;
 assert(meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status)&&n==2&&status==200&&initializations==1&&cleanups==1);
 c.http_origin="http://evil.example:8080";assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status)&&initializations==1);
 c.http_origin="http://example.com@evil.example";assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status));
 c.http_origin="http://example.com:8080/path";assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status));
 c.http_origin="http://example.com:8080";redirect_status=301;assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status)&&cleanups==2);redirect_status=200;
 headers_wait=1;read_wait=1;assert(meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status));
 headers_wait=4;assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status));headers_wait=1;cancel_on_wait=1;
 assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status));cancelled_flag=0;cancel_on_wait=0;
 memset(nonce,0,sizeof nonce);int before=initializations;assert(!meeting_time_http_fetch(&c,nonce,response,sizeof response,&n,&status)&&initializations==before);
 return 0;
}
