/** Duration from the Opus TOC, in 48 kHz samples; this does not decode audio. */
export function opusPacketSamples(first,second,length){
 if(!Number.isInteger(first)||!Number.isSafeInteger(length)||length<1||(first&4)!==0)throw Error('invalid_audio');
 const config=first>>3,code=first&3;
 const samples=config<12?[480,960,1920,2880][config&3]:config<16?[480,960][config&1]:[120,240,480,960][config&3];
 let frames;if(code===0)frames=1;else if(code===1||code===2)frames=2;else {if(length<2||!Number.isInteger(second))throw Error('invalid_audio');frames=second&63;if(frames===0)throw Error('invalid_audio')}
 const duration=samples*frames;if(duration>5760)throw Error('invalid_audio');return duration;
}
