import {validateOggOpus} from './ogg.mjs';
import {opusPacketSamples} from './opus-packet.mjs';
/** Validate without converting; upload confirmation must not allocate one page per frame. */
export function validateAudioStructure(input){
 if(!Buffer.isBuffer(input)||input.length<4)throw new Error('invalid_audio');
 if(input.subarray(0,4).toString()==='OggS'){const {durationSeconds}=validateOggOpus(input,{withDuration:true});return {format:'ogg',durationSeconds};}
 if(input.subarray(0,4).toString()!=='SCR1'||input.length<96)throw new Error('unsupported_audio');
 const frames=input.readUInt32LE(20);if(frames===0xffffffff||frames===0||frames>90000)throw new Error('invalid_audio');
 let pos=96;for(let i=0;i<frames;i++){if(pos>=input.length)throw new Error('invalid_audio');const len=input[pos++];if(len===0||len>160||pos+len>input.length)throw new Error('invalid_audio');if(opusPacketSamples(input[pos],len>1?input[pos+1]:undefined,len)!==960)throw new Error('invalid_audio');pos+=len}if(pos!==input.length)throw new Error('invalid_audio');return {format:'scr1',frames,durationSeconds:frames/50};
}
/** Convert SCR1 records (96-byte header, length-prefixed 20ms Opus) to Ogg Opus. */
export function toOgg(input){
 const info=validateAudioStructure(input);if(info.format==='ogg')return input;const frames=info.frames;
 const table=new Uint32Array(256);for(let i=0;i<256;i++){let r=i<<24;for(let j=0;j<8;j++)r=r&0x80000000?(r<<1)^0x04c11db7:r<<1;table[i]=r>>>0}
 const output=Buffer.alloc(input.length+27*frames+3);let offset=0;
 function page(packet,granule,seq,flags=0){if(packet.length>160)throw new Error('invalid_audio');const p=output.subarray(offset,offset+28+packet.length);offset+=p.length;p.write('OggS',0);p[5]=flags;p.writeBigUInt64LE(BigInt(granule),6);p.writeUInt32LE(0x41371234,14);p.writeUInt32LE(seq,18);p[26]=1;p[27]=packet.length;packet.copy(p,28);let crc=0;for(const b of p)crc=((crc<<8)^table[((crc>>>24)^b)&255])>>>0;p.writeUInt32LE(crc,22)}
 const head=Buffer.alloc(19);head.write('OpusHead');head[8]=1;head[9]=1;head.writeUInt32LE(16000,12);const tags=Buffer.alloc(24);tags.write('OpusTags');tags.writeUInt32LE(8,8);tags.write('passport',12);
 let pos=96;page(head,0,0,2);page(tags,0,1);for(let i=0;i<frames;i++){if(pos>=input.length)throw new Error('invalid_audio');const len=input[pos++];if(len===0||len>160||pos+len>input.length)throw new Error('invalid_audio');page(input.subarray(pos,pos+len),(i+1)*960,i+2,i===frames-1?4:0);pos+=len}if(pos!==input.length)throw new Error('invalid_audio');if(offset!==output.length)throw new Error('invalid_audio');return output;
}
