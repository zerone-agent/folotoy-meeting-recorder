import {test} from 'node:test';import {strict as assert} from 'node:assert';import {reconcileTranscript} from '../src/asr-stream.mjs';
test('stream output must cover independent full transcript and retain only bounded VAD times',()=>{
 const stream=[{text:'你好，世界。',startSeconds:1.2,endSeconds:4.4}];assert.deepEqual(reconcileTranscript('你好世界',stream,10),stream);
 assert.throws(()=>reconcileTranscript('你好世界再见',stream,10),/asr_incomplete/);
 assert.throws(()=>reconcileTranscript('你好世界',[{...stream[0],endSeconds:13}],10),/asr_invalid_event/);
 assert.throws(()=>reconcileTranscript('你好世界',[{...stream[0],endSeconds:0}],10),/asr_invalid_event/);
});
test('comparison preserves numerical symbols and rejects changed meaning',()=>{
 for(const [full,other] of [['1.5','15'],['10%','10'],['-3','3'],['1,000','1000'],['¥9','9']])assert.throws(()=>reconcileTranscript(full,[{text:other,startSeconds:0,endSeconds:1}],10),/asr_incomplete/);
});
test('late VAD segment is awaited and disconnect never publishes partial results',async()=>{
 const {EventEmitter}=await import('node:events'),{transcribeTimed}=await import('../src/asr-stream.mjs');
 const fetchImpl=async()=>new Response('data: {"type":"transcript.text.done","text":"甲乙"}\n\n',{status:200});
 class Fake extends EventEmitter {constructor(){super();queueMicrotask(()=>this.emit('open'))}send(raw){const m=JSON.parse(raw);if(m.type==='session.update')queueMicrotask(()=>this.emit('message',Buffer.from(JSON.stringify({type:'session.updated'}))));if(m.type==='input_audio_buffer.append'){const emit=e=>this.emit('message',Buffer.from(JSON.stringify(e)));emit({type:'input_audio_buffer.speech_started',item_id:'a',audio_start_ms:0});emit({type:'input_audio_buffer.speech_stopped',item_id:'a',audio_start_ms:1000});emit({type:'conversation.item.input_audio_transcription.completed',item_id:'a',transcript:'甲'});setTimeout(()=>{emit({type:'input_audio_buffer.speech_started',item_id:'b',audio_start_ms:2000});emit({type:'input_audio_buffer.speech_stopped',item_id:'b',audio_start_ms:3000});emit({type:'conversation.item.input_audio_transcription.completed',item_id:'b',transcript:'乙'});},300)}}close(){}}
 const result=await transcribeTimed(Buffer.from('a'),{apiKey:'k',durationSeconds:4,fetchImpl,WebSocketImpl:Fake,quietMs:50,settleMs:3000});assert.equal(result.map(x=>x.text).join(''),'甲乙');
 class Broken extends Fake{send(raw){if(JSON.parse(raw).type==='input_audio_buffer.append')queueMicrotask(()=>this.emit('close'));else super.send(raw)}}
 await assert.rejects(transcribeTimed(Buffer.from('a'),{apiKey:'k',durationSeconds:4,fetchImpl,WebSocketImpl:Broken,quietMs:50,settleMs:1000}),/asr_incomplete/);
});
test('minor wording differences remain visible for review without relaxing numeric checks',()=>{
 const item=[{text:'会议自言确认，项目明天开始实施。回顾计划。 ',startSeconds:0,endSeconds:1}];const result=reconcileTranscript('会议自研确认，项目明天开始实施。回顾计划。 ',item,2);
 assert.equal(result[0].comparison,'approximate');assert.equal(result[0].text,item[0].text);
 assert.throws(()=>reconcileTranscript('项目预算10%', [{text:'项目预算10',startSeconds:0,endSeconds:1}],2),/asr_incomplete/);
 assert.throws(()=>reconcileTranscript('会议决定取消项目并退款', [{text:'会议决定上线',startSeconds:0,endSeconds:1}],2),/asr_incomplete/);
});
