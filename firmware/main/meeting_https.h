#pragma once
#include "meeting_upload_protocol.h"
/* Inputs must remain valid for the whole request. CA is a PEM trust anchor.
 * time_valid must reflect a securely established clock, not unauthenticated SNTP. */
typedef struct {
 const char *origin; /* https://hostname[:port], without path, query or userinfo */
 const char *device_token;
 const char *ca_pem;
 void *cancel_ctx;
 bool (*is_cancelled)(void *ctx);
 bool (*time_valid)(void *ctx);
 void *client; /* owned by this session; closed by meeting_https_close */
 char tls_hostname[181]; /* owned by this session, retained by TLS transport */
} meeting_https_config;
void meeting_https_close(meeting_https_config *config);
bool meeting_https_bind(meeting_https_config *config, meeting_upload_transport *out);
