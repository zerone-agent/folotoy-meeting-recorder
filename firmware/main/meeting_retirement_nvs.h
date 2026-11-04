#pragma once
#include "meeting_retirement.h"
#include "nvs.h"
typedef struct {
    nvs_handle_t markers,snapshots,receipts,records,intents;
    uint32_t file_id;
    void *ctx;
    bool (*cancelled)(void *ctx);
    bool (*verify_completed)(void *ctx,const meeting_retired *id);
    bool (*files_absent)(void *ctx);
} meeting_retirement_nvs;
/* Each handle is open for read/write; caller serializes access. */
bool meeting_retirement_nvs_bind(meeting_retirement_nvs *state,meeting_retirement_io *io);
/* 0 present and valid, 1 missing, -1 corrupt or I/O error. */
int meeting_retirement_nvs_read(nvs_handle_t handle,uint32_t file_id,meeting_retired *out);
