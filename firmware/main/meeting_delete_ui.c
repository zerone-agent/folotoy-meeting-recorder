#include "meeting_delete_ui.h"
static bool valid_visibility(meeting_delete_visibility v){return (unsigned)v<=MEETING_DELETE_UNKNOWN;}
bool meeting_delete_ui_begin(meeting_delete_ui *ui,uint32_t file_id,meeting_delete_visibility visibility){
 if(!ui||ui->stage!=MEETING_DELETE_UI_IDLE||!file_id||!valid_visibility(visibility))return false;
 *ui=(meeting_delete_ui){.file_id=file_id,.visibility=visibility,.stage=MEETING_DELETE_UI_REVIEW};return true;
}
meeting_delete_ui_event meeting_delete_ui_confirm(meeting_delete_ui *ui,meeting_delete_visibility current,uint32_t *approved_id){
 if(!ui||!approved_id||!valid_visibility(current)||!ui->file_id||ui->stage==MEETING_DELETE_UI_IDLE)return MEETING_DELETE_UI_NONE;
 if(current!=ui->visibility){ui->visibility=current;ui->stage=MEETING_DELETE_UI_REVIEW;return MEETING_DELETE_UI_REFRESH;}
 if(ui->stage==MEETING_DELETE_UI_REVIEW){ui->stage=MEETING_DELETE_UI_CONFIRM;return MEETING_DELETE_UI_REFRESH;}
 if(ui->stage!=MEETING_DELETE_UI_CONFIRM)return MEETING_DELETE_UI_NONE;
 *approved_id=ui->file_id;*ui=(meeting_delete_ui){0};return MEETING_DELETE_UI_APPROVED;
}
void meeting_delete_ui_cancel(meeting_delete_ui *ui){if(ui)*ui=(meeting_delete_ui){0};}
