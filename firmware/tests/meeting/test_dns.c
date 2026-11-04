#include "meeting_dns.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include <assert.h>
#include <string.h>
static int64_t now;static bool cancel_flag,pending,fail_schedule;static dns_found_callback callback;static int callbacks;
int64_t esp_timer_get_time(void){return now;}
void vTaskDelay(unsigned ms){now+=(int64_t)ms*1000;if(now>=100000)cancel_flag=true;}
int ip4addr_aton(const char *s,ip4_addr_t *out){if(strcmp(s,"192.0.2.1"))return 0;out->addr=1;return 1;}
char *ip4addr_ntoa_r(const ip4_addr_t *ip,char *buf,int cap){assert(ip->addr==1&&cap>=10);strcpy(buf,"192.0.2.1");return buf;}
err_t tcpip_try_callback(tcpip_callback_fn cb,void *ctx){if(fail_schedule)return -1;cb(ctx);return ERR_OK;}
err_t dns_gethostbyname_addrtype(const char *name,ip_addr_t *addr,dns_found_callback cb,void *arg,uint8_t type){
 (void)arg;assert(!strcmp(name,"example.com")&&type==LWIP_DNS_ADDRTYPE_IPV4);callbacks++;if(pending){callback=cb;return ERR_INPROGRESS;}addr->is_v4=1;addr->ip.addr=1;return ERR_OK;
}
static bool cancelled(void *ctx){(void)ctx;return cancel_flag;}
int main(void){
 char ip[16];assert(meeting_dns_resolve("192.0.2.1",cancelled,NULL,1000000,ip)&&!strcmp(ip,"192.0.2.1"));
 assert(meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip)&&!strcmp(ip,"192.0.2.1"));
 pending=true;assert(!meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip));assert(cancel_flag&&now==100000);
 cancel_flag=false;int before=callbacks;assert(!meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip)&&callbacks==before);
 ip_addr_t resolved={{1},1};callback("example.com",&resolved,NULL);pending=false;now=0;
 assert(meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip)&&callbacks==before+1);
 fail_schedule=true;assert(!meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip));fail_schedule=false;
 assert(meeting_dns_resolve("example.com",cancelled,NULL,1000000,ip));return 0;
}
