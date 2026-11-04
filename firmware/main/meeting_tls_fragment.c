#include "mbedtls/ssl.h"

/* Keep all upstream defaults and verification settings; only request the
 * standardized maximum fragment length for stream clients. */
int __real_mbedtls_ssl_config_defaults(mbedtls_ssl_config *config, int endpoint, int transport, int preset);
int __wrap_mbedtls_ssl_config_defaults(mbedtls_ssl_config *config, int endpoint, int transport, int preset)
{
    int result=__real_mbedtls_ssl_config_defaults(config,endpoint,transport,preset);
    if(result!=0 || endpoint!=MBEDTLS_SSL_IS_CLIENT || transport!=MBEDTLS_SSL_TRANSPORT_STREAM)return result;
    return mbedtls_ssl_conf_max_frag_len(config,MBEDTLS_SSL_MAX_FRAG_LEN_4096);
}
