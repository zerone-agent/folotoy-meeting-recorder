#include "meeting_file_sequence.h"
#include <limits.h>
bool meeting_file_sequence_next(const meeting_file_sequence_io *io,uint32_t baseline,uint32_t wall_id,uint32_t *out)
{
    if(!io||!io->read||!io->write_sync||!out)return false;
    uint32_t current=0;int status=io->read(io->ctx,&current);
    if(status<0||status>1)return false;
    if(status==1||current<baseline)current=baseline;
    uint32_t candidate;
    if(wall_id>current)candidate=wall_id;
    else {if(current==UINT32_MAX)return false;candidate=current+1;}
    if(!candidate||!io->write_sync(io->ctx,candidate))return false;
    uint32_t checked=0;if(io->read(io->ctx,&checked)!=0||checked!=candidate)return false;
    *out=candidate;return true;
}
