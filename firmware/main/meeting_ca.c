#include "meeting_ca.h"
#include "mbedtls/sha256.h"
#include "mbedtls/x509_crt.h"
#include <string.h>

bool meeting_ca_validate_pem(const char *pem,uint8_t fingerprint[32],void *ctx)
{
    (void)ctx;if(!pem||!fingerprint)return false;
    size_t n=strlen(pem);if(!n||n>MEETING_CA_MAX)return false;
    /* Refuse extra certificates, private keys and trailing PEM material. */
    const char *begin="-----BEGIN CERTIFICATE-----",*end="-----END CERTIFICATE-----";
    if(strncmp(pem,begin,strlen(begin)))return false;
    const char *tail=strstr(pem,end);if(!tail||strstr(pem+strlen(begin),begin))return false;
    tail+=strlen(end);while(*tail=='\n'||*tail=='\r'||*tail==' '||*tail=='\t')tail++;
    if(*tail)return false;
    mbedtls_x509_crt crt;mbedtls_x509_crt_init(&crt);
    int ret=mbedtls_x509_crt_parse(&crt,(const unsigned char *)pem,n+1);
    bool ok=ret==0&&crt.next==NULL&&mbedtls_x509_crt_get_ca_istrue(&crt)&&crt.raw.p&&crt.raw.len&&
            mbedtls_sha256(crt.raw.p,crt.raw.len,fingerprint,0)==0;
    mbedtls_x509_crt_free(&crt);return ok;
}
