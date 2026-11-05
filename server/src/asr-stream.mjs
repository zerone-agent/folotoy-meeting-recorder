import WebSocket from 'ws';
import {randomUUID} from 'node:crypto';
import {transcribe} from './asr.mjs';
const normalize=s=>s.replace(/[\s，。！？；、《》]/gu,'');
/** Reject mismatched or invalid streams before a worker can publish anything. */
function boundedDistance(a,b,limit){
 if(Math.abs(a.length-b.length)>limit)return limit+1;let prev=Array(b.length+1).fill(limit+1);for(let j=0;j<=Math.min(b.length,limit);j++)prev[j]=j;
 for(let i=1;i<=a.length;i++){const cur=Array(b.length+1).fill(limit+1);if(i<=limit)cur[0]=i;let low=limit+1;for(let j=Math.max(1,i-limit);j<=Math.min(b.length,i+limit);j++){cur[j]=Math.min(prev[j]+1,cur[j-1]+1,prev[j-1]+(a[i-1]===b[j-1]?0:1));low=Math.min(low,cur[j])}if(low>limit)return limit+1;prev=cur}return prev[b.length];
}
export function reconcileTranscript(full,items,duration){
 if(typeof full!=='string'||!Array.isArray(items)||!items.length)throw Error('asr_incomplete');
 const stream=items.map(x=>x.text).join(''),a=Array.from(normalize(full)),b=Array.from(normalize(stream)),approximate=a.join('')!==b.join('');
 if(approximate){const numbers=s=>(s.match(/[+\-−￥¥$€£%％\d.,]+/gu)||[]).join('|');if(numbers(full)!==numbers(stream))throw Error('asr_incomplete');const limit=Math.min(100,Math.floor(a.length*0.1));if(boundedDistance(a,b,limit)>limit)throw Error('asr_incomplete')}
 let previous=0;for(const item of items){if(typeof item.text!=='string'||!Number.isFinite(item.startSeconds)||!Number.isFinite(item.endSeconds)||item.startSeconds<previous||item.endSeconds<item.startSeconds||item.endSeconds>duration+2)throw Error('asr_invalid_event');previous=item.startSeconds;}if(!approximate)return items;const result=items.map(item=>({...item,comparison:'approximate'}));result.fullText=full;return result;
}

/** Use VAD item bounds, not per-word alignments; never publish unmatched text. */
export async function transcribeTimed(audio,{apiKey,signal,onDelta,durationSeconds,WebSocketImpl=WebSocket,fetchImpl=fetch,settleMs=120000,quietMs=3000}={}){
 const full=await transcribe(audio,{apiKey,signal,onDelta,fetchImpl});if(signal?.aborted)throw Error('asr_incomplete');
 const items=await new Promise((resolve,reject)=>{
  const ws=new WebSocketImpl('wss://api.stepfun.com/v1/realtime/asr/stream',{headers:{Authorization:`Bearer ${apiKey}`}}),byId=new Map();let ended=false,sent=false,lastEvent=Date.now(),sentAt=0,timer;
  const finish=(error,value)=>{if(ended)return;ended=true;clearInterval(timer);signal?.removeEventListener('abort',abort);try{ws.close()}catch{}error?reject(error):resolve(value)};
  const abort=()=>finish(Error('asr_incomplete'));signal?.addEventListener('abort',abort,{once:true});
  const send=(type,body={})=>ws.send(JSON.stringify({event_id:randomUUID(),type,...body}));
  ws.on('open',()=>send('session.update',{session:{audio:{input:{format:{type:'ogg'},transcription:{model:'stepaudio-2.5-asr-stream',language:'zh',enable_itn:true},turn_detection:{type:'server_vad',silence_duration_ms:800,threshold:0.5}}}}}));
  let configured=false;ws.on('message',async raw=>{try{const e=JSON.parse(raw.toString());lastEvent=Date.now();if(e.type==='error')return finish(Error('asr_provider_error'));
   if(e.type==='session.updated'&&!configured){configured=true;for(let pos=0;pos<audio.length&&!ended;pos+=8000){send('input_audio_buffer.append',{audio:audio.subarray(pos,pos+8000).toString('base64')});await new Promise(r=>setTimeout(r,10))}sent=true;lastEvent=Date.now();sentAt=Date.now();}
   else if(e.type==='input_audio_buffer.speech_started'){if(typeof e.item_id!=='string'||byId.has(e.item_id)||!Number.isFinite(e.audio_start_ms))return finish(Error('asr_invalid_event'));byId.set(e.item_id,{startSeconds:e.audio_start_ms/1000});}
   else if(e.type==='input_audio_buffer.speech_stopped'){const x=byId.get(e.item_id);if(!x||!Number.isFinite(e.audio_start_ms))return finish(Error('asr_invalid_event'));x.endSeconds=e.audio_start_ms/1000;}
   else if(e.type==='conversation.item.input_audio_transcription.delta')onDelta?.();
   else if(e.type==='conversation.item.input_audio_transcription.completed'){const x=byId.get(e.item_id);if(!x||x.complete||typeof e.transcript!=='string')return finish(Error('asr_invalid_event'));x.text=e.transcript;x.complete=true;}
  }catch{finish(Error('asr_invalid_event'))}});
  ws.on('error',()=>finish(Error('asr_incomplete')));ws.on('close',()=>{if(!ended)finish(Error('asr_incomplete'))});
  timer=setInterval(()=>{if(!sent)return;const values=[...byId.values()];const complete=values.length&&values.every(x=>x.complete&&Number.isFinite(x.endSeconds));if(complete&&Date.now()-lastEvent>=quietMs){const collected=values.filter(x=>x.text?.trim()).sort((a,b)=>a.startSeconds-b.startSeconds).map(({text,startSeconds,endSeconds})=>({text,startSeconds,endSeconds}));try{const result=reconcileTranscript(full,collected,durationSeconds);if(!result.fullText||Date.now()-lastEvent>=10000)return finish(null,result)}catch(e){if(e.message!=='asr_incomplete')return finish(e)}}if(Date.now()-sentAt>=settleMs)finish(Error('asr_incomplete'));},250);
 });
 return reconcileTranscript(full,items,durationSeconds);
}
