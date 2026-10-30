import {opusPacketSamples} from './opus-packet.mjs';
/** Verify a single complete Ogg Opus stream before forwarding to ASR. */
export function validateOggOpus(data,{withDuration=false}={}){
 const table=new Uint32Array(256);for(let i=0;i<256;i++){let r=i<<24;for(let j=0;j<8;j++)r=r&0x80000000?(r<<1)^0x04c11db7:r<<1;table[i]=r>>>0}
 let offset=0,serial=null,seq=0,pending=false,eos=false,packetIndex=0,header=[],headerBytes=0,packetBytes=0,packetHead=[],totalSamples=0,preSkip=0,lastGranule=0n;
 while(offset<data.length){
  if(eos||data.length-offset<27||data.toString('ascii',offset,offset+4)!=='OggS'||data[offset+4]!==0)throw Error('invalid_audio');
  const flags=data[offset+5],n=data[offset+26],tableEnd=offset+27+n;if((flags&~7)!==0||tableEnd>data.length)throw Error('invalid_audio');
  const serialNow=data.readUInt32LE(offset+14),seqNow=data.readUInt32LE(offset+18),granule=data.readBigUInt64LE(offset+6);
  if(serial===null){if(!(flags&2)||seqNow!==0)throw Error('invalid_audio');serial=serialNow}else if(serialNow!==serial||flags&2)throw Error('invalid_audio');
  if(seqNow!==seq++||Boolean(flags&1)!==pending)throw Error('invalid_audio');
  let end=tableEnd;for(let i=0;i<n;i++)end+=data[offset+27+i];if(end>data.length)throw Error('invalid_audio');
  let crc=0;for(let i=offset;i<end;i++){const byte=i>=offset+22&&i<offset+26?0:data[i];crc=((crc<<8)^table[((crc>>>24)^byte)&255])>>>0}if(crc!==data.readUInt32LE(offset+22))throw Error('invalid_audio');
  let cursor=tableEnd;for(let i=0;i<n;i++){const size=data[offset+27+i];if(packetIndex>=2&&packetHead.length<2)for(let j=0;j<size&&packetHead.length<2;j++)packetHead.push(data[cursor+j]);packetBytes+=size;if(packetIndex<2){headerBytes+=size;if(headerBytes>65536)throw Error('invalid_audio');header.push(data.subarray(cursor,cursor+size))}cursor+=size;pending=size===255;
   if(!pending){if(packetIndex>=2){totalSamples+=opusPacketSamples(packetHead[0],packetHead[1],packetBytes);if(totalSamples>86400000+preSkip+5760)throw Error('invalid_audio')}if(packetIndex===0){const head=Buffer.concat(header);if(head.length<19||head.toString('ascii',0,8)!=='OpusHead'||head[8]!==1||head[9]!==1||head[18]!==0)throw Error('invalid_audio');preSkip=head.readUInt16LE(10)}else if(packetIndex===1){const tags=Buffer.concat(header);if(tags.length<16||tags.toString('ascii',0,8)!=='OpusTags')throw Error('invalid_audio');let p=12+tags.readUInt32LE(8);if(p+4>tags.length)throw Error('invalid_audio');const comments=tags.readUInt32LE(p);p+=4;if(comments>10000)throw Error('invalid_audio');for(let i=0;i<comments;i++){if(p+4>tags.length)throw Error('invalid_audio');const length=tags.readUInt32LE(p);p+=4+length;if(p>tags.length)throw Error('invalid_audio')}}header=[];headerBytes=0;packetBytes=0;packetHead=[];packetIndex++;}}
  if(granule!==0xffffffffffffffffn){if(granule<lastGranule)throw Error('invalid_audio');lastGranule=granule}
  if(flags&4){if(pending||packetIndex<3||granule===0xffffffffffffffffn||granule<BigInt(preSkip)||granule-BigInt(preSkip)>86400000n||granule>BigInt(totalSamples))throw Error('invalid_audio');eos=true}
  offset=end;
 }
 if(!eos||pending||offset!==data.length)throw Error('invalid_audio');return withDuration?{durationSeconds:Number(lastGranule-BigInt(preSkip))/48000}:data;
}
