import {validateOggOpus} from './ogg.mjs';

// Extend the recognizer input only. Keep the persisted recording and SSE input
// untouched; the extra silence gives server VAD time to close a final utterance.
export function appendVadSilence(audio, frames = 150) {
 // Removing EOS from a trimmed final page would expose samples intentionally
 // excluded by the source. Leave such streams unchanged instead.
 if (!validateOggOpus(audio,{withSampleInfo:true}).untrimmedEnd) return audio;
 if (!Number.isSafeInteger(frames) || frames < 1 || frames > 150) throw Error('invalid_audio');
 const table = new Uint32Array(256);
 for (let i=0;i<256;i++){let r=i<<24;for(let j=0;j<8;j++)r=r&0x80000000?(r<<1)^0x04c11db7:r<<1;table[i]=r>>>0}
 const checksum = page => {page.writeUInt32LE(0,22);let crc=0;for(const byte of page)crc=((crc<<8)^table[((crc>>>24)^byte)&255])>>>0;page.writeUInt32LE(crc,22)};
 let offset=0,last=0;while(offset<audio.length){last=offset;const n=audio[offset+26];let length=27+n;for(let i=0;i<n;i++)length+=audio[offset+27+i];offset+=length}
 const copy=Buffer.from(audio),serial=copy.readUInt32LE(last+14),granule=copy.readBigUInt64LE(last+6);let seq=copy.readUInt32LE(last+18);
 if(seq+frames>0xffffffff)throw Error('invalid_audio');copy[last+5]&=~4;checksum(copy.subarray(last));
 const pages=[copy];for(let i=1;i<=frames;i++){const page=Buffer.alloc(31);page.write('OggS');if(i===frames)page[5]=4;page.writeBigUInt64LE(granule+BigInt(i*960),6);page.writeUInt32LE(serial,14);page.writeUInt32LE(++seq,18);page[26]=1;page[27]=3;page.set([0xf8,0xff,0xfe],28);checksum(page);pages.push(page)}
 return Buffer.concat(pages);
}
