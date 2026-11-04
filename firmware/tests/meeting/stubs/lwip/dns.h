#pragma once
#include <stdint.h>
typedef int err_t;
typedef struct {uint32_t addr;} ip4_addr_t;
typedef struct {ip4_addr_t ip;int is_v4;} ip_addr_t;
typedef void (*dns_found_callback)(const char *,const ip_addr_t *,void *);
#define ERR_OK 0
#define ERR_INPROGRESS -5
#define LWIP_DNS_ADDRTYPE_IPV4 0
#define IP_IS_V4(a) ((a)->is_v4)
#define ip_2_ip4(a) (&(a)->ip)
err_t dns_gethostbyname_addrtype(const char *,ip_addr_t *,dns_found_callback,void *,uint8_t);
int ip4addr_aton(const char *,ip4_addr_t *);
char *ip4addr_ntoa_r(const ip4_addr_t *,char *,int);
