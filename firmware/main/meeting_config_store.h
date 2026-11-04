#pragma once
#include "meeting_config.h"
#include "meeting_config_fragments.h"

#define MEETING_CONFIG_RECORD_MAX (MEETING_CONFIG_MAX_BYTES + 12u)
typedef struct {
    void *ctx;
    /* read: 0 present, 1 absent, -1 error; size is set only when present. */
    int (*read)(void *ctx,uint8_t *out,size_t capacity,size_t *size);
    bool (*write_sync)(void *ctx,const uint8_t *data,size_t size);
} meeting_config_store_io;
typedef enum { MEETING_CONFIG_STORED, MEETING_CONFIG_ABSENT, MEETING_CONFIG_INVALID } meeting_config_store_status;
/* Scratch is private to one operation and must be wiped by the caller. */
meeting_config_store_status meeting_config_store_load(const meeting_config_store_io *io,meeting_ca_validate validate_ca,
                            void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *out);
/* Existing network/credential settings are never overwritten. A newer UTC
 * value for identical settings is accepted but is not stored here. Call only
 * after authenticating the live session and explicit user confirmation. */
bool meeting_config_store_commit(const meeting_config_store_io *io,const uint8_t *json,size_t length,
                                  bool authenticated,bool user_confirmed,meeting_ca_validate validate_ca,void *ca_ctx,
                                  uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *temporary);
/* Read-only preflight. A changed or unreadable existing record is rejected. */
bool meeting_config_store_compatible(const meeting_config_store_io *io,meeting_ca_validate validate_ca,
                                     void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],
                                     const meeting_config *expected,meeting_config *temporary);
/* expected must be the validated interpretation of this exact JSON. */
bool meeting_config_store_commit_preparsed(const meeting_config_store_io *io,const uint8_t *json,size_t length,
                                  meeting_ca_validate validate_ca,void *ca_ctx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],
                                  const meeting_config *expected,meeting_config *temporary);
