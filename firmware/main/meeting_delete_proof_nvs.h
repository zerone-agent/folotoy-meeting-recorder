#pragma once
#include "meeting_confirmation.h"
#include "nvs.h"
/* Explicit separate namespace: expected data cannot masquerade as an ack. */
typedef struct {nvs_handle_t handle;uint32_t file_id;bool opened;} meeting_delete_proof_nvs;
bool meeting_delete_proof_nvs_open(meeting_delete_proof_nvs *storage,uint32_t file_id,meeting_confirmation_io *io);
void meeting_delete_proof_nvs_close(meeting_delete_proof_nvs *storage);
