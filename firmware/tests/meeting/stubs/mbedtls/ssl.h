#pragma once
typedef struct { int marker; } mbedtls_ssl_config;
#define MBEDTLS_SSL_IS_CLIENT 0
#define MBEDTLS_SSL_IS_SERVER 1
#define MBEDTLS_SSL_TRANSPORT_STREAM 0
#define MBEDTLS_SSL_TRANSPORT_DATAGRAM 1
#define MBEDTLS_SSL_MAX_FRAG_LEN_4096 4
int mbedtls_ssl_conf_max_frag_len(mbedtls_ssl_config*, unsigned char);
