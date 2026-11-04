#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEETING_CA_MAX 4096u
typedef struct {
    char ssid[33],password[64],origin[201],device_token[513],ca_pem[MEETING_CA_MAX+1];
    /* Version 1 has no revocation identity; version 2 binds these to token. */
    char device_id[101],credential_id[65];
    uint8_t ca_sha256[32];
    int64_t utc;
    int16_t timezone_minutes;
} meeting_config;
/* CA validation must parse a single CA certificate and return the SHA-256 of
 * its DER representation. A parsed configuration is not yet authorized or
 * persistent: user time/fingerprint confirmation and durable commit follow. */
typedef bool (*meeting_ca_validate)(const char *pem,uint8_t fingerprint[32],void *ctx);
/* Revalidates a version 2 device/credential identity against its token. */
bool meeting_config_revocation_identity_valid(const meeting_config *config);
bool meeting_config_parse(const uint8_t *json,size_t length,meeting_ca_validate validate_ca,void *ctx,meeting_config *out);
void meeting_config_clear(meeting_config *config);
