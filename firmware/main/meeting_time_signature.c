#include "meeting_time_signature.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"
#include <string.h>

static int load_key(mbedtls_ecdsa_context *ec,const meeting_time_public_key *key)
{
 if(!key||key->public_key[0]!=4)return -1;
 int rc=mbedtls_ecp_group_load(&ec->MBEDTLS_PRIVATE(grp),MBEDTLS_ECP_DP_SECP256R1);
 if(rc==0)rc=mbedtls_ecp_point_read_binary(&ec->MBEDTLS_PRIVATE(grp),&ec->MBEDTLS_PRIVATE(Q),key->public_key,sizeof(key->public_key));
 if(rc==0)rc=mbedtls_ecp_check_pubkey(&ec->MBEDTLS_PRIVATE(grp),&ec->MBEDTLS_PRIVATE(Q));
 return rc;
}
bool meeting_time_public_key_valid(void *ctx,const meeting_time_public_key *key)
{
 (void)ctx;mbedtls_ecdsa_context ec;mbedtls_ecdsa_init(&ec);int rc=load_key(&ec,key);mbedtls_ecdsa_free(&ec);return rc==0;
}
bool meeting_time_signature_verify(void *ctx,const uint8_t *message,size_t length,const uint8_t *signature,size_t siglen)
{
 const meeting_time_public_key *key=ctx;
 if(!key||key->public_key[0]!=4||!message||!length||!signature||siglen<8||siglen>72)return false;
 uint8_t digest[32];mbedtls_ecdsa_context ec;mbedtls_ecdsa_init(&ec);
 int rc=load_key(&ec,key);
 if(rc==0)rc=mbedtls_sha256(message,length,digest,0);
 if(rc==0)rc=mbedtls_ecdsa_read_signature(&ec,digest,sizeof(digest),signature,siglen);
 memset(digest,0,sizeof(digest));mbedtls_ecdsa_free(&ec);return rc==0;
}
