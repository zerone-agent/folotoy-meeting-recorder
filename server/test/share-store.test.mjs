import {createHash} from 'node:crypto';
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {MeetingStore} from '../src/store.mjs';

test('shares latest minutes with explicit transcript permission and expiry',()=>{
 const s=new MeetingStore(':memory:');
 try {
  s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(JSON.stringify({versionId:'v1',segments:[{text:'secret'}]}),JSON.stringify({summary:'first'}));
  s.db.prepare("INSERT INTO minutes_versions(meeting_id,generation,transcript_version,data) VALUES('m',1,'v1',?)").run(JSON.stringify({summary:'first'}));
  const link=s.createShare('m',false,7,1000);
  assert.match(link.token,/^[A-Za-z0-9_-]{43}$/);
  assert.equal(link.expiresAt,1000+7*86400);
  assert.equal(s.readShare(link.token,1001).transcript,undefined);
  assert.match(s.readShare(link.token,1001).minutesMarkdown,/first/);
  assert.equal(s.readShare(link.token,link.expiresAt),null);
  s.db.prepare("UPDATE meetings SET state='deleting' WHERE id='m'").run();
  assert.equal(s.readShare(link.token,1001),null);
 } finally {s.close()}
});
test('share constraints and transcript permission are enforced',()=>{const s=new MeetingStore(':memory:');try{const t=JSON.stringify({versionId:'v',segments:[{text:'private'}]}),m=JSON.stringify({summary:'result'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(t,m);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(m);for(const days of [0,366,1.5])assert.throws(()=>s.createShare('m',false,days,100),/invalid/);const share=s.createShare('m',true,1,100);assert.equal(s.readShare(share.token,101).transcript,'private');s.db.prepare("UPDATE meetings SET state='queued',minutes=NULL WHERE id='m'").run();assert.deepEqual(s.readShare(share.token,101),{unavailable:true});s.db.prepare("UPDATE meetings SET state='completed',minutes=? WHERE id='m'").run(m);s.db.prepare("UPDATE meetings SET transcript=? WHERE id='m'").run(JSON.stringify({versionId:'other',segments:[{text:'wrong'}]}));assert.deepEqual(s.readShare(share.token,101),{unavailable:true});}finally{s.close()}});
test('startup rejects tampered share table schema',async()=>{const {mkdtempSync,rmSync}=await import('node:fs');const {tmpdir}=await import('node:os');const {join}=await import('node:path');const dir=mkdtempSync(join(tmpdir(),'share-schema-')),path=join(dir,'db');try{let s=new MeetingStore(path);s.db.exec('DROP TABLE meeting_shares; CREATE TABLE meeting_shares(token_hash TEXT,meeting_id TEXT,include_transcript INTEGER,expires_at INTEGER)');s.close();assert.throws(()=>new MeetingStore(path),/invalid_share_schema/)}finally{rmSync(dir,{recursive:true,force:true})}});
test('share read holds one snapshot across concurrent deletion',async()=>{const {mkdtempSync,rmSync}=await import('node:fs');const {tmpdir}=await import('node:os');const {join}=await import('node:path');const dir=mkdtempSync(join(tmpdir(),'share-race-')),path=join(dir,'db');try{const s=new MeetingStore(path),other=new MeetingStore(path);try{const t=JSON.stringify({versionId:'v',segments:[{text:'private'}]}),m=JSON.stringify({summary:'result'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(t,m);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(m);const share=s.createShare('m',true,1);const original=s.db.prepare.bind(s.db);let interleaved=false;s.db.prepare=sql=>{const stmt=original(sql);if(sql.startsWith('SELECT s.include_transcript'))return {get(...args){const row=stmt.get(...args);other.beginDelete('d','m');interleaved=true;return row}};return stmt};assert.equal(s.readShare(share.token).transcript,'private');assert.equal(interleaved,true);assert.equal(s.readShare(share.token),null)}finally{s.close();other.close()}}finally{rmSync(dir,{recursive:true,force:true})}});
test('share read never mixes results across concurrent retranscription',async()=>{const {mkdtempSync,rmSync}=await import('node:fs');const {tmpdir}=await import('node:os');const {join}=await import('node:path');const dir=mkdtempSync(join(tmpdir(),'share-race-')),path=join(dir,'db');try{const s=new MeetingStore(path),other=new MeetingStore(path);try{const t=JSON.stringify({versionId:'v',segments:[{text:'old transcript'}]}),m=JSON.stringify({summary:'old minutes'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes,chunks) VALUES('m','d','completed',?,?,1)").run(t,m);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(m);const share=s.createShare('m',true,1);const original=s.db.prepare.bind(s.db);let once=true;s.db.prepare=sql=>{const stmt=original(sql);if(sql.startsWith('SELECT s.include_transcript'))return {get(...args){const row=stmt.get(...args);if(once){once=false;other.adminRetranscribe('m')}return row}};return stmt};const old=s.readShare(share.token);assert.match(old.minutesMarkdown,/old minutes/);assert.equal(old.transcript,'old transcript');assert.deepEqual(s.readShare(share.token),{unavailable:true})}finally{s.close();other.close()}}finally{rmSync(dir,{recursive:true,force:true})}});
test('startup rejects orphaned share records',async()=>{const {mkdtempSync,rmSync}=await import('node:fs');const {tmpdir}=await import('node:os');const {join}=await import('node:path');const {DatabaseSync}=await import('node:sqlite');const dir=mkdtempSync(join(tmpdir(),'share-orphan-')),path=join(dir,'db');try{new MeetingStore(path).close();const db=new DatabaseSync(path);db.exec('PRAGMA foreign_keys=OFF');db.prepare('INSERT INTO meeting_shares(token_hash,meeting_id,include_transcript,expires_at) VALUES(?,?,?,?)').run('a'.repeat(64),'missing',0,123);db.close();assert.throws(()=>new MeetingStore(path),/invalid_share_state/)}finally{rmSync(dir,{recursive:true,force:true})}});
test('deletion stays pending if a share record cannot be removed',()=>{const s=new MeetingStore(':memory:');try{const m=JSON.stringify({summary:'result'});s.db.prepare("INSERT INTO meetings(id,owner,state,minutes) VALUES('m','d','completed',?)").run(m);s.createShare('m',false,1);s.db.exec("CREATE TRIGGER retain_share BEFORE DELETE ON meeting_shares BEGIN SELECT RAISE(IGNORE); END");s.beginDelete('d','m');assert.throws(()=>s.finishDelete('m'),/deletion_incomplete/);assert.equal(s.db.prepare("SELECT state FROM meetings WHERE id='m'").get().state,'deleting')}finally{s.close()}});
test('share shows the newly published version and respects each link permission',()=>{const s=new MeetingStore(':memory:');try{const t1=JSON.stringify({versionId:'v1',segments:[{text:'first transcript'}]}),m1=JSON.stringify({summary:'first minutes'}),t2=JSON.stringify({versionId:'v2',segments:[{text:'second transcript'}]}),m2=JSON.stringify({summary:'second minutes'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(t1,m1);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v1',?)").run(m1);const without=s.createShare('m',false,7),withTranscript=s.createShare('m',true,7);s.db.prepare("UPDATE meetings SET transcript=?,minutes=? WHERE id='m'").run(t2,m2);s.db.prepare("INSERT INTO minutes_versions VALUES('m',2,'v2',?)").run(m2);const first=s.readShare(without.token),second=s.readShare(withTranscript.token);assert.match(first.minutesMarkdown,/second minutes/);assert.equal(first.transcript,undefined);assert.equal(second.transcript,'second transcript');assert.notEqual(without.token,withTranscript.token)}finally{s.close()}});
test('sharing reflects saved manual edits to the latest minutes',()=>{const s=new MeetingStore(':memory:');try{const t=JSON.stringify({versionId:'v',segments:[{text:'source'}]}),m=JSON.stringify({summary:'generated'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(t,m);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(m);const link=s.createShare('m',false,7);s.adminSaveMinutes('m','# corrected minutes',0,createHash('sha256').update(m).digest('hex'));assert.match(s.readShare(link.token).minutesMarkdown,/corrected minutes/)}finally{s.close()}});

test('retranscription shares current generated minutes instead of an edit based on old results',()=>{
 const s=new MeetingStore(':memory:');try{
  const audio=Buffer.from('test audio'),digest=createHash('sha256').update(audio).digest('hex');
  s.create('d','m');s.put('d','m',0,audio,digest);s.finalize('d','m',{count:1,bytes:audio.length,digest,reason:'user'});
  const oldTranscript=JSON.stringify({versionId:'old',segments:[{id:'s0',text:'old transcript'}]}),oldMinutes=JSON.stringify({summary:'old generated'});
  s.publish(s.claim('m','queued'),oldTranscript);s.publish(s.claim('m','minutes_queued'),oldMinutes);
  s.adminSaveMinutes('m','# old edited minutes',0,createHash('sha256').update(oldMinutes).digest('hex'));
  const link=s.createShare('m',true,7);s.adminRetranscribe('m');assert.deepEqual(s.readShare(link.token),{unavailable:true});
  const currentTranscript=JSON.stringify({versionId:'new',segments:[{id:'s0',text:'new transcript'}]}),currentMinutes=JSON.stringify({summary:'new generated'});
  s.publish(s.claim('m','queued'),currentTranscript);s.publish(s.claim('m','minutes_queued'),currentMinutes);
  const shared=s.readShare(link.token);assert.equal(shared.transcript,'new transcript');assert.match(shared.minutesMarkdown,/new generated/);assert.doesNotMatch(shared.minutesMarkdown,/old edited minutes/);
 }finally{s.close()}
});

test('retranscription makes an edit stale even if regenerated minutes are identical',()=>{
 const s=new MeetingStore(':memory:');try{
  const audio=Buffer.from('audio'),digest=createHash('sha256').update(audio).digest('hex');s.create('d','m');s.put('d','m',0,audio,digest);s.finalize('d','m',{count:1,bytes:audio.length,digest,reason:'user'});
  const minutes=JSON.stringify({summary:'same generated minutes'});s.publish(s.claim('m','queued'),JSON.stringify({versionId:'old',segments:[{id:'s0',text:'old'}]}));s.publish(s.claim('m','minutes_queued'),minutes);s.adminSaveMinutes('m','# old edit',0,createHash('sha256').update(minutes).digest('hex'));
  const link=s.createShare('m',true,7);s.adminRetranscribe('m');s.publish(s.claim('m','queued'),JSON.stringify({versionId:'new',segments:[{id:'s0',text:'new'}]}));s.publish(s.claim('m','minutes_queued'),minutes);
  const shared=s.readShare(link.token);assert.equal(shared.transcript,'new');assert.match(shared.minutesMarkdown,/same generated minutes/);assert.doesNotMatch(shared.minutesMarkdown,/old edit/);assert.equal(s.adminGet('m').minutes_edited_stale,true);
 }finally{s.close()}
});

test('legacy edits without a transcript version are not mixed with a shared transcript',()=>{
 const s=new MeetingStore(':memory:');try{
  const transcript=JSON.stringify({versionId:'v',segments:[{text:'current transcript'}]}),minutes=JSON.stringify({summary:'current minutes'});
  s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(transcript,minutes);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(minutes);
  s.db.prepare('INSERT INTO minutes_edits(meeting_id,markdown,revision,source_hash) VALUES(?,?,?,?)').run('m','# unverified edit',1,createHash('sha256').update(minutes).digest('hex'));
  const shared=s.readShare(s.createShare('m',true,7).token);assert.equal(shared.transcript,'current transcript');assert.match(shared.minutesMarkdown,/current minutes/);assert.doesNotMatch(shared.minutesMarkdown,/unverified edit/);assert.equal(shared.minutesEditedStale,true);
 }finally{s.close()}
});

test('existing version 1 shares migrate with downloading disabled',async()=>{
 const {mkdtempSync,rmSync}=await import('node:fs'),{tmpdir}=await import('node:os'),{join}=await import('node:path');const dir=mkdtempSync(join(tmpdir(),'share-v1-')),path=join(dir,'db');
 try{let s=new MeetingStore(path);const transcript=JSON.stringify({versionId:'v',segments:[{text:'text'}]}),minutes=JSON.stringify({summary:'minutes'});s.db.prepare("INSERT INTO meetings(id,owner,state,transcript,minutes) VALUES('m','d','completed',?,?)").run(transcript,minutes);s.db.prepare("INSERT INTO minutes_versions VALUES('m',1,'v',?)").run(minutes);const link=s.createShare('m',true,7);s.db.exec("ALTER TABLE meeting_shares DROP COLUMN allow_download; UPDATE service_metadata SET value='1' WHERE key='share_schema'");s.close();s=new MeetingStore(path);assert.equal(s.readShare(link.token).allowDownload,false);assert.equal(s.db.prepare("SELECT value FROM service_metadata WHERE key='share_schema'").get().value,'2');s.close()}finally{rmSync(dir,{recursive:true,force:true})}
});
