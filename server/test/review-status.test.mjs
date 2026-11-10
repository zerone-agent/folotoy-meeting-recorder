import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {MeetingStore} from '../src/store.mjs';
import {createMeetingRouter} from '../src/routes.mjs';

test('review is bound to the displayed minutes and transcript and clears after editing',async()=>{
 const store=new MeetingStore(':memory:');try{
  const audio=Buffer.from('audio'),digest=createHash('sha256').update(audio).digest('hex');store.create('d','m');store.put('d','m',0,audio,digest);store.finalize('d','m',{count:1,bytes:audio.length,digest,reason:'user'});
  store.publish(store.claim('m','queued'),JSON.stringify({versionId:'v1',segments:[{id:'s0',text:'transcript'}]}));const minutes=JSON.stringify({summary:'minutes'});store.publish(store.claim('m','minutes_queued'),minutes);
  const app=createMeetingRouter(store,{adminToken:'admin123'}),headers={authorization:'Bearer admin123','content-type':'application/json'},get=()=>app.request('/admin/meetings/m/result',{headers});
  let result=await (await get()).json();assert.equal(result.reviewed,false);const link=store.createShare('m',true,7);
  assert.equal((await app.request('/admin/meetings/m/review',{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({reviewKey:result.reviewKey})})).status,401);
  assert.equal((await app.request('/admin/meetings/m/review',{method:'POST',headers,body:JSON.stringify({reviewKey:result.reviewKey})})).status,200);
  result=await (await get()).json();assert.equal(result.reviewed,true);assert.equal(store.readShare(link.token).reviewed,true);
  assert.equal(store.adminList().meetings[0].reviewed,true);assert.equal(store.adminPage().meetings[0].reviewed,true);
  store.adminSaveMinutes('m','# updated',0,createHash('sha256').update(minutes).digest('hex'));result=await (await get()).json();assert.equal(result.reviewed,false);assert.equal(store.readShare(link.token).reviewed,false);assert.equal(store.adminList().meetings[0].reviewed,false);assert.equal(store.adminPage().meetings[0].reviewed,false);
  assert.equal((await app.request('/admin/meetings/m/review',{method:'POST',headers,body:JSON.stringify({reviewKey:'a'.repeat(64)})})).status,409);
 }finally{store.close()}
});

test('retranscribing invalidates review even if the minutes text is unchanged',()=>{
 const store=new MeetingStore(':memory:');try{
  const audio=Buffer.from('audio'),digest=createHash('sha256').update(audio).digest('hex');store.create('d','m');store.put('d','m',0,audio,digest);store.finalize('d','m',{count:1,bytes:audio.length,digest,reason:'user'});const minutes=JSON.stringify({summary:'same'});
  store.publish(store.claim('m','queued'),JSON.stringify({versionId:'old',segments:[{id:'s0',text:'old'}]}));store.publish(store.claim('m','minutes_queued'),minutes);store.markReviewed('m',store.adminGet('m').reviewKey);const link=store.createShare('m',true,7);
  store.adminRetranscribe('m');assert.deepEqual(store.readShare(link.token),{unavailable:true});store.publish(store.claim('m','queued'),JSON.stringify({versionId:'new',segments:[{id:'s0',text:'new'}]}));store.publish(store.claim('m','minutes_queued'),minutes);
  assert.equal(store.adminGet('m').reviewed,false);assert.equal(store.readShare(link.token).reviewed,false);
 }finally{store.close()}
});

test('review notices never become stale text in edits, shares or downloads',async()=>{
 const store=new MeetingStore(':memory:');try{
  const audio=Buffer.from('audio'),digest=createHash('sha256').update(audio).digest('hex');store.create('d','m');store.put('d','m',0,audio,digest);store.finalize('d','m',{count:1,bytes:audio.length,digest,reason:'user'});
  store.publish(store.claim('m','queued'),JSON.stringify({versionId:'v',segments:[{id:'s0',text:'transcript'}]}));const minutes=JSON.stringify({summary:'minutes'});store.publish(store.claim('m','minutes_queued'),minutes);
  const app=createMeetingRouter(store,{adminToken:'admin123'}),admin={authorization:'Bearer admin123','content-type':'application/json'},read=async()=>await (await app.request('/admin/meetings/m/result',{headers:admin})).json(),sha=createHash('sha256').update(minutes).digest('hex');
  let current=await read();assert.doesNotMatch(current.minutesMarkdown,/未经人工校对|已人工复核/);
  store.adminSaveMinutes('m',current.minutesMarkdown+'\n> 自动转录和纪要未经人工校对；引用定位正确不代表结论一定得到原文支持。\n',0,sha);
  current=await read();store.markReviewed('m',current.reviewKey);const link=store.createShare('m',true,7,Math.floor(Date.now()/1000),true),auth={authorization:'Bearer '+link.token};
  let shared=await (await app.request('/share/data',{headers:auth})).json(),download=await (await app.request('/share/download/minutes',{headers:auth})).text();assert.equal(shared.reviewed,true);assert.doesNotMatch(shared.minutesMarkdown,/未经人工校对|已人工复核/);assert.doesNotMatch(download,/未经人工校对|已人工复核/);
  current=await read();store.adminSaveMinutes('m',current.minutesMarkdown+'\n> 当前纪要已人工复核。\n',1,sha);
  shared=await (await app.request('/share/data',{headers:auth})).json();download=await (await app.request('/share/download/minutes',{headers:auth})).text();assert.equal(shared.reviewed,false);assert.doesNotMatch(shared.minutesMarkdown,/未经人工校对|已人工复核/);assert.doesNotMatch(download,/已人工复核/);assert.match(download,/未经人工校对/);
 }finally{store.close()}
});
