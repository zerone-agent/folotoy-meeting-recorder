#include "mbedtls/ssl.h"
#include <assert.h>
static int defaults_result,fragment_result,defaults_calls,fragment_calls;
int __wrap_mbedtls_ssl_config_defaults(mbedtls_ssl_config*,int,int,int);
int __real_mbedtls_ssl_config_defaults(mbedtls_ssl_config *c,int e,int t,int p)
{(void)c;(void)e;(void)t;(void)p;defaults_calls++;return defaults_result;}
int mbedtls_ssl_conf_max_frag_len(mbedtls_ssl_config *c,unsigned char value)
{(void)c;assert(value==MBEDTLS_SSL_MAX_FRAG_LEN_4096);fragment_calls++;return fragment_result;}
int main(void)
{
 mbedtls_ssl_config c={0};
 assert(__wrap_mbedtls_ssl_config_defaults(&c,0,0,0)==0&&defaults_calls==1&&fragment_calls==1);
 assert(__wrap_mbedtls_ssl_config_defaults(&c,1,0,0)==0&&fragment_calls==1);
 assert(__wrap_mbedtls_ssl_config_defaults(&c,0,1,0)==0&&fragment_calls==1);
 defaults_result=-10;assert(__wrap_mbedtls_ssl_config_defaults(&c,0,0,0)==-10&&fragment_calls==1);
 defaults_result=0;fragment_result=-11;assert(__wrap_mbedtls_ssl_config_defaults(&c,0,0,0)==-11&&fragment_calls==2);
 return 0;
}
