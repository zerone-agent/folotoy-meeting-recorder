#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Only the signed-time endpoint is requested over HTTP. No credentials are
 * accepted by this interface; HTTPS uploads remain separately verified. */
typedef struct {const char *https_origin,*http_origin;bool (*cancelled)(void *ctx);void *cancel_ctx;} meeting_time_http;
bool meeting_time_http_fetch(void *ctx,const uint8_t nonce[32],char *response,size_t capacity,size_t *length,int *status);
