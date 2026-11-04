#include "meeting_file_sequence.h"
#include <assert.h>
#include <limits.h>
typedef struct {uint32_t value;int present,read_error,write_error,wrong_read,writes;} fixture;
static int read_id(void *ctx,uint32_t *out){fixture *f=ctx;if(f->read_error)return -1;if(!f->present)return 1;*out=f->value+(f->wrong_read?1u:0u);return 0;}
static bool write_id(void *ctx,uint32_t value){fixture *f=ctx;f->writes++;f->value=value;f->present=1;return !f->write_error;}
int main(void)
{
    fixture f={0};meeting_file_sequence_io io={&f,read_id,write_id};uint32_t id=99;
    assert(meeting_file_sequence_next(&io,100,0,&id)&&id==101&&f.value==101);
    assert(meeting_file_sequence_next(&io,100,500,&id)&&id==500&&f.value==500);
    f.write_error=1;assert(!meeting_file_sequence_next(&io,100,0,&id)&&id==500&&f.value==501);
    f.write_error=0;assert(meeting_file_sequence_next(&io,100,0,&id)&&id==502);
    f.read_error=1;assert(!meeting_file_sequence_next(&io,100,0,&id)&&id==502);f.read_error=0;
    f.value=UINT_MAX;assert(!meeting_file_sequence_next(&io,100,0,&id));
    f.value=502;f.wrong_read=1;assert(!meeting_file_sequence_next(&io,100,0,&id)&&id==502);
    return 0;
}
