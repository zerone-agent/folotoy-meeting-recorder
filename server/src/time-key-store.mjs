import {createHash,createPrivateKey,generateKeyPairSync} from 'node:crypto';
import {createTimeProofSigner,timeProofPublicKey,validateTimeProofOrigin} from './time-proof.mjs';

const MARKER='time_signer_v1';
const MIN_EPOCH=1735689600, MAX_EPOCH=4102444800;
const validEpoch=n=>Number.isSafeInteger(n)&&n>=MIN_EPOCH&&n<=MAX_EPOCH;
const keyId=point=>createHash('sha256').update(point).digest('hex');
const SCHEMA='CREATE TABLE IF NOT EXISTS time_signer (singleton INTEGER PRIMARY KEY CHECK(singleton=1), origin TEXT NOT NULL, key_id TEXT NOT NULL, private_pem TEXT NOT NULL, last_epoch INTEGER NOT NULL, max_forward INTEGER NOT NULL)';

function loadState(db){
 const marker=db.prepare('SELECT value FROM service_metadata WHERE key=?').get(MARKER)?.value;
 const table=!!db.prepare("SELECT 1 FROM sqlite_master WHERE type='table' AND name='time_signer'").get();
 if(!table){if(marker)throw Error('time_key_state_invalid');return null}
 const rows=db.prepare('SELECT singleton,origin,key_id,private_pem,last_epoch,max_forward FROM time_signer').all();
 if(!marker&&!rows.length)return null;
 if(!marker||rows.length!==1||rows[0].singleton!==1)throw Error('time_key_state_invalid');
 const row=rows[0];
 if(marker!==row.key_id||!validEpoch(row.last_epoch)||!Number.isSafeInteger(row.max_forward)||row.max_forward<1)throw Error('time_key_state_invalid');
 let privateKey,point;
 try{validateTimeProofOrigin(row.origin);privateKey=createPrivateKey(row.private_pem);point=timeProofPublicKey(privateKey)}catch{throw Error('time_key_state_invalid')}
 if(keyId(point)!==row.key_id)throw Error('time_key_state_invalid');
 return {origin:row.origin,id:row.key_id,privateKey,privatePem:row.private_pem,point,lastEpoch:row.last_epoch,maxForward:row.max_forward};
}

/** Validate a snapshot without returning secret key material. */
export function validateTimeKeyState(db){loadState(db);return true;}

/** Caller owns the target transaction and a consistent read transaction on
 * safety. Preserve the latest known signing key and issuance floor. Rotation
 * conflicts and a missing latest key fail closed. */
export function reconcileTimeKeyState(safety,target){
 const current=loadState(safety),saved=loadState(target);
 if(!current){if(saved)throw Error('time_key_missing_from_safety');return;}
 if(saved&&(saved.id!==current.id||saved.origin!==current.origin||saved.maxForward!==current.maxForward||saved.lastEpoch>current.lastEpoch))throw Error('time_key_recovery_conflict');
 target.exec(SCHEMA);
 target.prepare('INSERT OR REPLACE INTO time_signer(singleton,origin,key_id,private_pem,last_epoch,max_forward) VALUES(1,?,?,?,?,?)').run(current.origin,current.id,current.privatePem,current.lastEpoch,current.maxForward);
 target.prepare('INSERT OR REPLACE INTO service_metadata(key,value) VALUES(?,?)').run(MARKER,current.id);
}

/** Candidate local key store. Caller must authenticate initial provisioning,
 * supply a verified initial clock and choose a maximum forward step. Key bytes
 * live in the database: protect it and its backups as secrets. This module
 * does not expose an HTTP endpoint, perform rotation, or prevent DB rollback. */
export class TimeKeyStore {
 #db; #maxForwardSeconds; #monotonicSeconds; #bootMonotonic; #lastMonotonic; #anchorEpoch;
 constructor(db,{maxForwardSeconds,monotonicSeconds=()=>performance.now()/1000}={}) {
  if(!db||!Number.isSafeInteger(maxForwardSeconds)||maxForwardSeconds<1||typeof monotonicSeconds!=='function')throw Error('invalid_time_policy');
  const now=monotonicSeconds();if(!Number.isFinite(now)||now<0)throw Error('invalid_time_policy');
  this.#db=db;this.#maxForwardSeconds=maxForwardSeconds;this.#monotonicSeconds=monotonicSeconds;
  this.#bootMonotonic=now;this.#lastMonotonic=now;this.#anchorEpoch=null;
  db.exec(SCHEMA);
 }
 #tx(fn){this.#db.exec('BEGIN IMMEDIATE');try{const v=fn();this.#db.exec('COMMIT');return v}catch(e){try{this.#db.exec('ROLLBACK')}catch{}throw e}}
 #read() {
  const state=loadState(this.#db);
  if(state&&state.maxForward!==this.#maxForwardSeconds)throw Error('time_key_state_invalid');
  return state;
 }
 /** One-time creation; never replaces an existing or damaged key. */
 initialize(origin,verifiedEpoch){
  validateTimeProofOrigin(origin);if(!validEpoch(verifiedEpoch))throw Error('invalid_time');
  const started=this.#monotonicSeconds();
  if(!Number.isFinite(started)||started<this.#lastMonotonic)throw Error('time_monotonic_invalid');
  const result=this.#tx(()=>{
   const current=this.#read();
   if(current){if(current.origin!==origin)throw Error('time_origin_conflict');return {info:this.#publicInfo(current),created:false}}
   const {privateKey}=generateKeyPairSync('ec',{namedCurve:'prime256v1'});
   const point=timeProofPublicKey(privateKey),id=keyId(point),pem=privateKey.export({type:'pkcs8',format:'pem'});
   this.#db.prepare('INSERT INTO time_signer(singleton,origin,key_id,private_pem,last_epoch,max_forward) VALUES(1,?,?,?,?,?)').run(origin,id,pem,verifiedEpoch,this.#maxForwardSeconds);
   this.#db.prepare('INSERT INTO service_metadata(key,value) VALUES(?,?)').run(MARKER,id);
   return {info:this.#publicInfo({origin,id,point}),created:true};
  });
  if(result.created){this.#anchorEpoch=verifiedEpoch;this.#bootMonotonic=started;this.#lastMonotonic=started;}
  return result.info;
 }
 #publicInfo(state){return {origin:state.origin,keyId:state.id,publicKey:Buffer.from(state.point)};}
 getPublic(){const state=this.#read();return state?this.#publicInfo(state):null;}
 #allowed(state,epoch){
  const now=this.#monotonicSeconds();
  if(!Number.isFinite(now)||now<this.#lastMonotonic)throw Error('time_monotonic_invalid');
  this.#lastMonotonic=now;
  if(this.#anchorEpoch===null)this.#anchorEpoch=state.lastEpoch;
  const elapsed=Math.floor(now-this.#bootMonotonic);
  if(epoch<state.lastEpoch||epoch>this.#anchorEpoch+elapsed+this.#maxForwardSeconds)throw Error('time_out_of_policy');
 }
 #advance(state,epoch){
  const changed=this.#db.prepare('UPDATE time_signer SET last_epoch=? WHERE singleton=1 AND key_id=? AND last_epoch=?').run(epoch,state.id,state.lastEpoch);
  if(changed.changes!==1)throw Error('time_key_state_invalid');
 }
 /** Trusted server clock checkpoint prevents a normally running but idle
  * service from exhausting its restart/offline allowance. */
 checkpoint(epoch){
  if(!validEpoch(epoch))throw Error('invalid_time');
  return this.#tx(()=>{const state=this.#read();if(!state)return false;this.#allowed(state,epoch);this.#advance(state,epoch);return true});
 }
 /** Signing and the monotonic issuance floor commit atomically. A forward
  * jump above policy fails closed; administrative recovery is separate. */
 sign(nonce,epoch){
  if(!validEpoch(epoch))throw Error('invalid_time');
  return this.#tx(()=>{
   const state=this.#read();if(!state)throw Error('time_key_missing');
   this.#allowed(state,epoch);
   const proof=createTimeProofSigner(state.privateKey,state.origin)(nonce,epoch);
   this.#advance(state,epoch);
   return {...proof,keyId:state.id};
  });
 }
}
