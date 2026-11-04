#include "meeting_receipt.h"
#include <string.h>
static bool valid(const meeting_manifest *m)
{
    if (!m || m->id[36]!='\0' || !m->count || m->count>8192 || !m->bytes || m->bytes>20000000 || m->reason>3 || m->time_trust>2) return false;
    for (unsigned i=0;i<36;i++) {
        char c=m->id[i];
        if(i==8||i==13||i==18||i==23){if(c!='-')return false;}
        else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    }
    if (m->recorded_at_present && (m->recorded_at<946684800 || m->recorded_at>4102444800LL)) return false;
    if (!m->recorded_at_present && m->time_trust==2) return false;
    return true;
}
bool meeting_receipt_matches(const meeting_manifest *a,const meeting_manifest *b,bool committed)
{
    if(!committed || !valid(a) || !valid(b))return false;
    return !memcmp(a->id,b->id,37) && a->count==b->count && a->bytes==b->bytes &&
           !memcmp(a->digest,b->digest,32) && a->reason==b->reason &&
           a->recorded_at_present==b->recorded_at_present &&
           (!a->recorded_at_present || a->recorded_at==b->recorded_at) && a->time_trust==b->time_trust;
}
