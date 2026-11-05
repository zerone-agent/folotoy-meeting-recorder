import {test} from 'node:test';
import {strict as assert} from 'node:assert';
import {createHash} from 'node:crypto';
import {MeetingStore} from '../src/store.mjs';
test('manual minutes persist, reject stale revisions, and are removed on deletion',()=>{
 const store=new MeetingStore(':memory:');try{
  store.create('device','meeting');
  store.db.prepare("UPDATE meetings SET state='completed',minutes=? WHERE id='meeting'").run(JSON.stringify({summary:'original'}));
  const saved=store.adminSaveMinutes('meeting','# Edited',0,createHash('sha256').update(JSON.stringify({summary:'original'})).digest('hex'));assert.equal(saved.revision,1);
  assert.equal(store.adminGet('meeting').edited_minutes,'# Edited');
  assert.throws(()=>store.adminSaveMinutes('meeting','# stale',0,createHash('sha256').update(JSON.stringify({summary:'original'})).digest('hex')),/conflict/);
  store.adminBeginDelete('meeting');store.finishDelete('meeting');
  assert.equal(store.db.prepare('SELECT COUNT(*) AS n FROM minutes_edits').get().n,0);
 }finally{store.close()}
});

test('deletion fails closed when database retains a manual edit',()=>{
 const store=new MeetingStore(':memory:');try{store.create('device','meeting');store.db.prepare("UPDATE meetings SET state='completed',minutes=? WHERE id='meeting'").run(JSON.stringify({summary:'original'}));store.adminSaveMinutes('meeting','# Edited',0,createHash('sha256').update(JSON.stringify({summary:'original'})).digest('hex'));store.db.exec('CREATE TRIGGER retain_edit BEFORE DELETE ON minutes_edits BEGIN SELECT RAISE(IGNORE); END');store.adminBeginDelete('meeting');assert.throws(()=>store.finishDelete('meeting'),/deletion_incomplete/);assert.equal(store.db.prepare('SELECT state FROM meetings WHERE id=?').get('meeting').state,'deleting');}finally{store.close()}
});
test('admin save is authenticated, version fenced and returned by markdown download',async()=>{
 const {createMeetingRouter}=await import('../src/routes.mjs');const store=new MeetingStore(':memory:');try{
  store.create('device','meeting');const original=JSON.stringify({summary:'old'});store.db.prepare("UPDATE meetings SET state='completed',minutes=? WHERE id='meeting'").run(original);
  const app=createMeetingRouter(store,{adminToken:'meeting-record'}),headers={'Authorization':'Bearer meeting-record','Content-Type':'application/json'},body=JSON.stringify({markdown:'# Revised',revision:0,sourceHash:createHash('sha256').update(original).digest('hex')});
  assert.equal((await app.request('/admin/meetings/meeting/minutes',{method:'PUT',headers:{'Content-Type':'application/json'},body})).status,401);
  assert.equal((await app.request('/admin/meetings/meeting/minutes',{method:'PUT',headers,body})).status,200);
  assert.equal((await app.request('/admin/meetings/meeting/minutes',{method:'PUT',headers,body})).status,409);
  const download=await app.request('/admin/meetings/meeting/minutes.md',{headers});assert.equal(await download.text(),'# Revised');
 }finally{store.close()}
});
test('manual edit rejects an obsolete generated-minutes version',()=>{
 const store=new MeetingStore(':memory:');try{store.create('d','m');const old=JSON.stringify({summary:'old'});store.db.prepare("UPDATE meetings SET state='completed',minutes=? WHERE id='m'").run(old);const stale=createHash('sha256').update(old).digest('hex');store.db.prepare('UPDATE meetings SET minutes=? WHERE id=?').run(JSON.stringify({summary:'new'}),'m');assert.throws(()=>store.adminSaveMinutes('m','# stale',0,stale),/conflict/);}finally{store.close()}
});
