#include "meeting_scr1.h"
#include <assert.h>
#include <string.h>
typedef struct {const uint8_t *data;size_t length;int fail;} fixture;
static int read_at(void *c,uint64_t at,uint8_t *out,size_t n){fixture *f=c;if(f->fail || at>f->length || n>f->length-at)return -1;memcpy(out,f->data+at,n);return (int)n;}
int main(void){uint8_t b[105]={0};memcpy(b,"SCR1",4);b[4]=1;memset(b+20,255,4);b[96]=3;b[97]=0xf8;b[98]=0xff;b[99]=0xfe;b[100]=3;b[101]=0xf8;b[102]=0xff;b[103]=0xfe;b[104]=3;fixture f={b,sizeof b,0};meeting_scr1_scan_result r={0};assert(meeting_scr1_scan(read_at,&f,sizeof b,&r));assert(r.frames==2&&r.valid_end==104&&r.incomplete_tail);assert(meeting_scr1_scan(read_at,&f,104,&r)&&!r.incomplete_tail);b[20]=2;b[21]=b[22]=b[23]=0;assert(meeting_scr1_scan(read_at,&f,104,&r));assert(!meeting_scr1_scan(read_at,&f,105,&r));b[20]=1;assert(!meeting_scr1_scan(read_at,&f,104,&r));b[20]=2;b[101]=0;assert(!meeting_scr1_scan(read_at,&f,104,&r));b[101]=0xf8;f.fail=1;assert(!meeting_scr1_scan(read_at,&f,104,&r));return 0;}
