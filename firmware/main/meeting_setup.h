#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#define MEETING_SETUP_ERR_CLOCK 0x7101

/* All calls belong to one non-UI task. The caller stops legacy BLE/Wi-Fi,
 * disallows setup while recording, and renders the entire fingerprint and
 * UTC value before accepting a physical confirmation. */
typedef enum { MEETING_SETUP_IDLE, MEETING_SETUP_PAIRING, MEETING_SETUP_REVIEW,
               MEETING_SETUP_FIRST_CLOCK, MEETING_SETUP_SAVED } meeting_setup_state;
typedef struct {
    meeting_setup_state state;
    char code[13];
    uint8_t ca_sha256[32];
    int64_t utc;
} meeting_setup_view;
typedef enum { MEETING_SETUP_NO_RESULT, MEETING_SETUP_NETWORK_STARTED, MEETING_SETUP_NETWORK_FAILED } meeting_setup_result;
esp_err_t meeting_setup_start(void);
void meeting_setup_poll(void);
void meeting_setup_view_get(meeting_setup_view *view);
esp_err_t meeting_setup_confirm(void);
esp_err_t meeting_setup_cancel(void);
meeting_setup_result meeting_setup_take_result(void);
