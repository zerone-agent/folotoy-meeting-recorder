import {test} from 'node:test';
import {strict as assert} from 'node:assert';
import {createHash} from 'node:crypto';
import {mkdtempSync,rmSync} from 'node:fs';
import {join} from 'node:path';
import {tmpdir} from 'node:os';
import {MeetingStore} from '../src/store.mjs';
import {createMeetingRouter} from '../src/routes.mjs';
import {drainOnce} from '../src/worker.mjs';
import {validateOggOpus} from '../src/ogg.mjs';

const hash=b=>createHash('sha256').update(b).digest('hex');
function recording(frames){const header=Buffer.alloc(96);header.write('SCR1');header.writeUInt32LE(frames,20);const data=Buffer.alloc(frames*4);for(let i=0;i<frames;i++)data.set([3,0xf8,0xff,0xfe],i*4);return Buffer.concat([header,data])}
test('30-minute recording can be uploaded in blocks, confirmed and recovered after restart',async()=>{
 const dir=mkdtempSync(join(tmpdir(),'meeting-30-upload-')),db=join(dir,'meeting.sqlite'),token='d'.repeat(40),admin='a'.repeat(40),data=recording(90000),digest=hash(data),auth={authorization:`Bearer ${token}`};let store;
 try{
  store=new MeetingStore(db);store.registerDevice('device',token);let app=createMeetingRouter(store,{adminToken:admin});
  assert.equal((await app.request('/meetings/m',{method:'POST',headers:auth})).status,201);
  let count=0;for(let offset=0;offset<data.length;offset+=65536){const block=data.subarray(offset,offset+65536);const response=await app.request(`/meetings/m/chunks/${count++}`,{method:'PUT',headers:{...auth,'x-content-sha256':hash(block)},body:block});assert.equal(response.status,201)}
  const manifest={count,bytes:data.length,digest,reason:'limit'};let response=await app.request('/meetings/m/finalize',{method:'POST',headers:auth,body:JSON.stringify(manifest)});assert.equal(response.status,200);assert.equal((await response.json()).committed,true);
  store.close();store=new MeetingStore(db);app=createMeetingRouter(store,{adminToken:admin});response=await app.request('/meetings/m',{headers:auth});const result=await response.json();assert.equal(result.committed,true);assert.equal(result.manifest.digest,digest);assert.equal(result.manifest.reason,'limit');assert.equal(result.chunks.length,count);assert.ok(result.chunks.every(c=>c.valid));assert.deepEqual(store.adminAudio('m'),data);
  let submitted=false;const adapters={asr:async audio=>{assert.equal(audio.subarray(0,4).toString(),'OggS');validateOggOpus(audio);submitted=true;return '开始、中间和结束的模拟内容'},minutes:async transcript=>({summary:transcript.segments[0].text,discussion:[],decisions:[],actions:[],openQuestions:[]})};
  await drainOnce(store,adapters);await drainOnce(store,adapters);assert.equal(submitted,true);
  response=await app.request('/admin/meetings/m/result',{headers:{authorization:`Bearer ${admin}`}});const processed=await response.json();assert.equal(processed.state,'completed');assert.equal(processed.captureStatus,'truncated');assert.equal(processed.minutes.incomplete,true);assert.equal(processed.minutes.incompleteReason,'limit');assert.ok(processed.minutes.reviewReasons.includes('meeting_incomplete'));
 }finally{store?.close();rmSync(dir,{recursive:true,force:true})}
});
