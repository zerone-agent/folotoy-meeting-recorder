#pragma once
#include <stdbool.h>
#include <stdint.h>
/* These reflect only locally verified knowledge; unknown must never be called
 * unuploaded because the server may have committed a lost response. */
typedef enum {MEETING_DELETE_UNCONFIRMED,MEETING_DELETE_CONFIRMED,MEETING_DELETE_UNKNOWN} meeting_delete_visibility;
typedef enum {MEETING_DELETE_UI_IDLE,MEETING_DELETE_UI_REVIEW,MEETING_DELETE_UI_CONFIRM} meeting_delete_ui_stage;
typedef struct {uint32_t file_id;meeting_delete_visibility visibility;meeting_delete_ui_stage stage;} meeting_delete_ui;
typedef enum {MEETING_DELETE_UI_NONE,MEETING_DELETE_UI_REFRESH,MEETING_DELETE_UI_APPROVED} meeting_delete_ui_event;
bool meeting_delete_ui_begin(meeting_delete_ui *ui,uint32_t file_id,meeting_delete_visibility visibility);
/* If knowledge changed between confirmations, return to review. The caller
 * must serialize/recheck the status when it creates the actual intent. */
meeting_delete_ui_event meeting_delete_ui_confirm(meeting_delete_ui *ui,meeting_delete_visibility current,uint32_t *approved_id);
void meeting_delete_ui_cancel(meeting_delete_ui *ui);
