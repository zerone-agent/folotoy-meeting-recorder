#include <mbedtls/sha256.h>
void mbedtls_sha256_init(mbedtls_sha256_context *c){c->ctx=EVP_MD_CTX_new();}
int mbedtls_sha256_starts(mbedtls_sha256_context *c,int sha224){return sha224||!c->ctx||EVP_DigestInit_ex(c->ctx,EVP_sha256(),0)!=1?-1:0;}
int mbedtls_sha256_update(mbedtls_sha256_context *c,const unsigned char *p,size_t n){return EVP_DigestUpdate(c->ctx,p,n)==1?0:-1;}
int mbedtls_sha256_finish(mbedtls_sha256_context *c,unsigned char out[32]){unsigned n=0;return EVP_DigestFinal_ex(c->ctx,out,&n)==1&&n==32?0:-1;}
void mbedtls_sha256_free(mbedtls_sha256_context *c){EVP_MD_CTX_free(c->ctx);c->ctx=0;}
