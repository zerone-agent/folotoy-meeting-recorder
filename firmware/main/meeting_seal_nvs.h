#pragma once
#include "meeting_seal.h"
#include "nvs.h"
typedef enum {MEETING_SEAL_OK,MEETING_SEAL_MISSING,MEETING_SEAL_ERROR} meeting_seal_status;
meeting_seal_status meeting_seal_nvs_read(nvs_handle_t nvs,uint32_t file_id,meeting_seal *out);
/* Caller must commit only after successful source and index sync and close. */
bool meeting_seal_nvs_commit(nvs_handle_t nvs,const meeting_seal *seal);
