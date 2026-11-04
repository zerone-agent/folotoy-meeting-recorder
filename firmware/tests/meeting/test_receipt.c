#include "meeting_receipt.h"
#include <assert.h>
#include <string.h>
int main(void){meeting_manifest a={.count=2,.bytes=100,.reason=3,.time_trust=0},b;memcpy(a.id,"12345678-1234-4123-8123-123456789abc",37);memset(a.digest,7,32);b=a;assert(meeting_receipt_matches(&a,&b,true));assert(!meeting_receipt_matches(&a,&b,false));b.digest[2]^=1;assert(!meeting_receipt_matches(&a,&b,true));b=a;b.count++;assert(!meeting_receipt_matches(&a,&b,true));b=a;b.recorded_at_present=true;b.recorded_at=1700000000;assert(!meeting_receipt_matches(&a,&b,true));b=a;b.time_trust=2;assert(!meeting_receipt_matches(&a,&b,true));b=a;b.id[36]='x';assert(!meeting_receipt_matches(&a,&b,true));return 0;}
