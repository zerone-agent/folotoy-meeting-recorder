import test from 'node:test';
import assert from 'node:assert/strict';
import {createPublicKey,verify} from 'node:crypto';
import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {DatabaseSync} from 'node:sqlite';
import {TimeKeyStore} from '../src/time-key-store.mjs';
import {MeetingStore} from '../src/store.mjs';
import {backupDatabase} from '../src/backup.mjs';
const origin='https://meeting.example',nonce=Buffer.alloc(32,7),policy={maxForwardSeconds:300};
function db(){const d=new DatabaseSync(':memory:');d.exec('CREATE TABLE service_metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL)');return d}
function publicKey(point){return createPublicKey({key:{kty:'EC',crv:'P-256',x:point.subarray(1,33).toString('base64url'),y:point.subarray(33,65).toString('base64url')},format:'jwk'})}

test('key and issuance floor survive restart and reject rollback or unexpected jumps',()=>{
 const d=db();try{
  let keys=new TimeKeyStore(d,policy);assert.equal(keys.getPublic(),null);assert.throws(()=>keys.sign(nonce,1800000000),/time_key_missing/);
  const pub=keys.initialize(origin,1800000000);assert.equal(pub.keyId.length,64);assert.equal(pub.publicKey.length,65);
  const proof=keys.sign(nonce,1800000001);assert.equal(proof.keyId,pub.keyId);assert.ok(verify('sha256',proof.message,publicKey(pub.publicKey),proof.signature));
  keys=new TimeKeyStore(d,policy);assert.deepEqual(keys.initialize(origin,1800000000),pub);assert.throws(()=>keys.initialize('https://other.example',1800000000),/time_origin_conflict/);
  assert.throws(()=>keys.sign(nonce,1800000000),/time_out_of_policy/);assert.throws(()=>keys.sign(nonce,1800000302),/time_out_of_policy/);
  assert.throws(()=>keys.sign(Buffer.alloc(32),1800000002),/invalid_time_proof/);
  assert.equal(d.prepare('SELECT last_epoch FROM time_signer').get().last_epoch,1800000001);
  assert.equal(keys.sign(nonce,1800000002).keyId,pub.keyId);
  assert.throws(()=>new TimeKeyStore(d,{maxForwardSeconds:301}).getPublic(),/time_key_state_invalid/);
 }finally{d.close()}
});

test('missing, mismatched or damaged key state never creates a replacement',()=>{
 for(const damage of ["DELETE FROM time_signer","DELETE FROM service_metadata WHERE key='time_signer_v1'","UPDATE time_signer SET private_pem='broken'","UPDATE time_signer SET key_id='bad'"]){
  const d=db();try{const keys=new TimeKeyStore(d,policy);keys.initialize(origin,1800000000);d.exec(damage);
   assert.throws(()=>keys.getPublic(),/time_key_state_invalid/);assert.throws(()=>keys.initialize(origin,1800000000),/time_key_state_invalid/);
  }finally{d.close()}
 }
});

test('failed issuance-floor commit never returns a proof or advances state',()=>{
 const d=db();try{
  const keys=new TimeKeyStore(d,policy);keys.initialize(origin,1800000000);
  d.exec("CREATE TRIGGER fail_time_floor BEFORE UPDATE ON time_signer BEGIN SELECT RAISE(ABORT,'write failed'); END");
  assert.throws(()=>keys.sign(nonce,1800000001),/write failed/);
  assert.equal(d.prepare('SELECT last_epoch FROM time_signer').get().last_epoch,1800000000);
  d.exec('DROP TRIGGER fail_time_floor');assert.ok(keys.sign(nonce,1800000001).signature.length);
 }finally{d.close()}
});

test('monotonic uptime permits normal long idle without accumulating forward tolerance',()=>{
 const d=db();try{
  let mono=0;const keys=new TimeKeyStore(d,{...policy,monotonicSeconds:()=>mono});keys.initialize(origin,1800000000);
  mono=40*86400;const current=1800000000+mono;
  assert.equal(keys.sign(nonce,current).message.readBigUInt64BE(68),BigInt(current));
  assert.equal(keys.checkpoint(current+299),true);
  assert.throws(()=>keys.sign(nonce,current+301),/time_out_of_policy/);
  mono--;assert.throws(()=>keys.checkpoint(current+300),/time_monotonic_invalid/);
  const restarted=new TimeKeyStore(d,{...policy,monotonicSeconds:()=>0});
  assert.throws(()=>restarted.sign(nonce,current+600),/time_out_of_policy/);
 }finally{d.close()}
});

test('uptime before first key provisioning does not expand forward tolerance',()=>{
 const d=db();try{
  let mono=0;const keys=new TimeKeyStore(d,{...policy,monotonicSeconds:()=>mono});
  mono=40*86400;keys.initialize(origin,1800000000);
  assert.throws(()=>keys.sign(nonce,1800000301),/time_out_of_policy/);
  assert.equal(keys.sign(nonce,1800000300).message.readBigUInt64BE(68),1800000300n);
 }finally{d.close()}
});

test('consistent backup retains the signing identity and issuance floor',()=>{
 const dir=mkdtempSync(join(tmpdir(),'meeting-time-key-'));let live,copy;
 try{
  const source=join(dir,'live.db'),backup=join(dir,'backup.db');live=new MeetingStore(source);
  const keys=new TimeKeyStore(live.db,policy),pub=keys.initialize(origin,1800000000);keys.sign(nonce,1800000002);
  backupDatabase({sourcePath:source,outputPath:backup});live.close();live=null;
  copy=new MeetingStore(backup);const restored=new TimeKeyStore(copy.db,policy);
  assert.deepEqual(restored.getPublic(),pub);assert.throws(()=>restored.sign(nonce,1800000001),/time_out_of_policy/);assert.equal(restored.sign(nonce,1800000003).keyId,pub.keyId);
 }finally{live?.close();copy?.close();rmSync(dir,{recursive:true,force:true})}
});

test('restore replays a newly provisioned key and latest issuance floor from safety',async()=>{
 const {restoreDatabase}=await import('../src/restore.mjs');
 const dir=mkdtempSync(join(tmpdir(),'meeting-time-restore-'));let live,restored;
 try{
  const source=join(dir,'live.db'),backup=join(dir,'backup.db'),out=join(dir,'out.db');live=new MeetingStore(source);
  backupDatabase({sourcePath:source,outputPath:backup});
  const keys=new TimeKeyStore(live.db,policy),pub=keys.initialize(origin,1800000000);keys.sign(nonce,1800000002);live.close();live=null;
  restoreDatabase({backupPath:backup,safetyPath:source,outputPath:out});restored=new MeetingStore(out);
  const recovered=new TimeKeyStore(restored.db,policy);assert.deepEqual(recovered.getPublic(),pub);
  assert.throws(()=>recovered.sign(nonce,1800000001),/time_out_of_policy/);assert.equal(recovered.sign(nonce,1800000003).keyId,pub.keyId);
 }finally{live?.close();restored?.close();rmSync(dir,{recursive:true,force:true})}
});

test('restore advances an existing key floor to the latest safety state',async()=>{
 const {restoreDatabase}=await import('../src/restore.mjs');
 const dir=mkdtempSync(join(tmpdir(),'meeting-time-restore-'));let live,restored;
 try{
  const source=join(dir,'live.db'),backup=join(dir,'backup.db'),out=join(dir,'out.db');live=new MeetingStore(source);
  const keys=new TimeKeyStore(live.db,policy),pub=keys.initialize(origin,1800000000);keys.sign(nonce,1800000001);
  backupDatabase({sourcePath:source,outputPath:backup});keys.sign(nonce,1800000002);live.close();live=null;
  restoreDatabase({backupPath:backup,safetyPath:source,outputPath:out});restored=new MeetingStore(out);
  const recovered=new TimeKeyStore(restored.db,policy);assert.deepEqual(recovered.getPublic(),pub);
  assert.throws(()=>recovered.sign(nonce,1800000001),/time_out_of_policy/);assert.equal(recovered.sign(nonce,1800000003).keyId,pub.keyId);
 }finally{live?.close();restored?.close();rmSync(dir,{recursive:true,force:true})}
});

test('restore refuses missing or conflicting latest signing identity',async()=>{
 const {restoreDatabase}=await import('../src/restore.mjs');
 for(const mode of ['missing','conflicting','corrupt']){
  const dir=mkdtempSync(join(tmpdir(),'meeting-time-restore-'));let live;
  try{
   const source=join(dir,'live.db'),backup=join(dir,'backup.db'),out=join(dir,'out.db');live=new MeetingStore(source);
   new TimeKeyStore(live.db,policy).initialize(origin,1800000000);backupDatabase({sourcePath:source,outputPath:backup});
   if(mode==='corrupt')live.db.exec("UPDATE time_signer SET private_pem='broken'");
   else {live.db.exec("DELETE FROM time_signer; DELETE FROM service_metadata WHERE key='time_signer_v1'");if(mode==='conflicting')new TimeKeyStore(live.db,policy).initialize(origin,1800000000)}
   live.close();live=null;
   assert.throws(()=>restoreDatabase({backupPath:backup,safetyPath:source,outputPath:out}),/time_key_(missing_from_safety|recovery_conflict|state_invalid)/);
  }finally{live?.close();rmSync(dir,{recursive:true,force:true})}
 }
});

test('backup rejects damaged signing state',()=>{
 const dir=mkdtempSync(join(tmpdir(),'meeting-time-backup-'));let live;
 try{const source=join(dir,'live.db'),backup=join(dir,'backup.db');live=new MeetingStore(source);new TimeKeyStore(live.db,policy).initialize(origin,1800000000);live.db.exec("UPDATE time_signer SET private_pem='broken'");assert.throws(()=>backupDatabase({sourcePath:source,outputPath:backup}),/time_key_state_invalid/)}
 finally{live?.close();rmSync(dir,{recursive:true,force:true})}
});
