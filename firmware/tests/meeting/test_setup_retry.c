#include "meeting_setup_retry.h"
#include <assert.h>
int main(void)
{
    meeting_setup_retry r={0};
    assert(!meeting_setup_retry_due(&r,0,false));
    assert(meeting_setup_retry_due(&r,0,true));
    assert(!meeting_setup_retry_due(&r,29999,true));
    assert(!meeting_setup_retry_due(&r,30000,false));
    assert(meeting_setup_retry_due(&r,30000,true));
    assert(!meeting_setup_retry_due(&r,100,true));
    assert(!meeting_setup_retry_due(&r,30099,true));
    assert(meeting_setup_retry_due(&r,30100,true));
    assert(!meeting_setup_retry_due(&r,-1,true));
    return 0;
}
