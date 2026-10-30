import {toOgg} from '../src/audio.mjs';
export function sampleOgg(){const b=Buffer.alloc(100);b.write('SCR1');b.writeUInt32LE(1,20);b[96]=3;b.set([0xf8,0xff,0xfe],97);return toOgg(b)}
