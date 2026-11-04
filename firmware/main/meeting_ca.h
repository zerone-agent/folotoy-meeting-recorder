#pragma once
#include "meeting_config.h"
/* Hashes the DER form of exactly one parsed CA certificate. */
bool meeting_ca_validate_pem(const char *pem,uint8_t fingerprint[32],void *ctx);
