#include "meeting_dns.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include <stdatomic.h>
#include <string.h>
enum {DNS_IDLE,DNS_PENDING,DNS_DONE};
static atomic_int state;
static char name[181],answer[16];
static bool success;
static void done(const char *host,const ip_addr_t *address,void *ctx){
 (void)host;(void)ctx;success=address&&IP_IS_V4(address)&&ip4addr_ntoa_r(ip_2_ip4(address),answer,sizeof answer)!=NULL;
 atomic_store_explicit(&state,DNS_DONE,memory_order_release);
}
static void start(void *ctx){
 (void)ctx;ip_addr_t address;
 err_t err=dns_gethostbyname_addrtype(name,&address,done,NULL,LWIP_DNS_ADDRTYPE_IPV4);
 if(err==ERR_OK)done(name,&address,NULL);else if(err!=ERR_INPROGRESS)done(name,NULL,NULL);
}
bool meeting_dns_resolve(const char *hostname,bool (*cancelled)(void *),void *ctx,int64_t deadline,char ipv4[16]){
 if(!hostname||!cancelled||!ipv4||cancelled(ctx)||esp_timer_get_time()>=deadline)return false;
 size_t n=strlen(hostname);if(!n||n>=sizeof name)return false;
 ip4_addr_t literal;if(ip4addr_aton(hostname,&literal))return ip4addr_ntoa_r(&literal,ipv4,16)!=NULL;
 int old=atomic_load_explicit(&state,memory_order_acquire);
 if(old==DNS_PENDING)return false;
 if(old==DNS_DONE)atomic_store_explicit(&state,DNS_IDLE,memory_order_relaxed);
 memcpy(name,hostname,n+1);success=false;atomic_store_explicit(&state,DNS_PENDING,memory_order_release);
 if(tcpip_try_callback(start,NULL)!=ERR_OK){atomic_store_explicit(&state,DNS_IDLE,memory_order_release);return false;}
 while(atomic_load_explicit(&state,memory_order_acquire)==DNS_PENDING){
  if(cancelled(ctx)||esp_timer_get_time()>=deadline)return false;
  vTaskDelay(pdMS_TO_TICKS(50));
 }
 if(cancelled(ctx)||esp_timer_get_time()>=deadline){atomic_store_explicit(&state,DNS_IDLE,memory_order_release);return false;}
 bool ok=success;if(ok)memcpy(ipv4,answer,sizeof answer);
 atomic_store_explicit(&state,DNS_IDLE,memory_order_release);return ok;
}
