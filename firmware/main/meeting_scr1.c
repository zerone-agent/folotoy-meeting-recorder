#include "meeting_scr1.h"

bool meeting_scr1_project(uint64_t offset, uint8_t *buffer, size_t length,
                          uint64_t valid_end, uint32_t frames)
{
    if ((!buffer && length) || valid_end < 96 || !frames || frames > 90000 ||
        offset > valid_end || length > valid_end - offset) return false;
    for (size_t i = 0; i < length; ++i) {
        uint64_t pos = offset + i;
        if (pos >= 20 && pos < 24) buffer[i] = (uint8_t)(frames >> ((pos - 20) * 8));
        else if (pos >= 24 && pos < 72) buffer[i] = 0;
    }
    return true;
}

static bool valid_packet(const uint8_t *p, size_t n)
{
    if (!n || (p[0] & 4)) return false;
    unsigned config=p[0]>>3, code=p[0]&3, samples;
    static const unsigned silk[]={480,960,1920,2880};
    static const unsigned celt[]={120,240,480,960};
    if(config<12) samples=silk[config&3];
    else if(config<16) samples=(config&1)?960:480;
    else samples=celt[config&3];
    unsigned count=code==0?1:code<3?2:n>1?(p[1]&63):0;
    return count && samples*count==960;
}

bool meeting_scr1_scan(meeting_scr1_read_at read_at, void *ctx, uint64_t file_size,
                        meeting_scr1_scan_result *out)
{
    if (!read_at || !out || file_size < 96) return false;
    uint8_t header[96], packet[160], size;
    if (read_at(ctx,0,header,sizeof(header)) != (int)sizeof(header) ||
        header[0]!='S' || header[1]!='C' || header[2]!='R' || header[3]!='1' ||
        header[4]!=1 || header[5] || header[6] || header[7]) return false;
    uint32_t declared = (uint32_t)header[20] | ((uint32_t)header[21]<<8) |
                        ((uint32_t)header[22]<<16) | ((uint32_t)header[23]<<24);
    if (declared != UINT32_MAX && (declared == 0 || declared > 90000)) return false;
    meeting_scr1_scan_result result={.valid_end=96,.frames=0,.incomplete_tail=false};
    uint64_t pos=96;
    while(pos<file_size){
        if(result.frames>=90000 || read_at(ctx,pos,&size,1)!=1 || size==0 || size>160) return false;
        if((uint64_t)size>file_size-pos-1){result.incomplete_tail=true;break;}
        if(read_at(ctx,pos+1,packet,size)!=(int)size || !valid_packet(packet,size))return false;
        pos+=1u+size;result.valid_end=pos;result.frames++;
    }
    if(!result.frames || (declared != UINT32_MAX &&
       (result.incomplete_tail || result.frames != declared))) return false;
    *out=result;return true;
}
