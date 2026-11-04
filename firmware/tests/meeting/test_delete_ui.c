#include "meeting_delete_ui.h"
#include <assert.h>
int main(void){
 meeting_delete_ui ui={0};uint32_t id=0;
 assert(!meeting_delete_ui_begin(&ui,0,MEETING_DELETE_UNKNOWN));
 assert(meeting_delete_ui_begin(&ui,7,MEETING_DELETE_UNKNOWN));
 assert(!meeting_delete_ui_begin(&ui,8,MEETING_DELETE_CONFIRMED));
 assert(meeting_delete_ui_confirm(&ui,MEETING_DELETE_UNKNOWN,&id)==MEETING_DELETE_UI_REFRESH&&ui.stage==MEETING_DELETE_UI_CONFIRM&&!id);
 assert(meeting_delete_ui_confirm(&ui,MEETING_DELETE_CONFIRMED,&id)==MEETING_DELETE_UI_REFRESH&&ui.stage==MEETING_DELETE_UI_REVIEW&&!id);
 assert(meeting_delete_ui_confirm(&ui,MEETING_DELETE_CONFIRMED,&id)==MEETING_DELETE_UI_REFRESH&&ui.stage==MEETING_DELETE_UI_CONFIRM);
 assert(meeting_delete_ui_confirm(&ui,MEETING_DELETE_CONFIRMED,&id)==MEETING_DELETE_UI_APPROVED&&id==7&&ui.stage==MEETING_DELETE_UI_IDLE);
 assert(meeting_delete_ui_begin(&ui,8,MEETING_DELETE_UNCONFIRMED));meeting_delete_ui_cancel(&ui);
 assert(meeting_delete_ui_confirm(&ui,MEETING_DELETE_UNCONFIRMED,&id)==MEETING_DELETE_UI_NONE);
 return 0;
}
