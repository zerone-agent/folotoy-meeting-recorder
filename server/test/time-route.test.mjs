import test from 'node:test';
import assert from 'node:assert/strict';
import {createPublicKey,verify} from 'node:crypto';
import {MeetingStore} from '../src/store.mjs';
import {TimeKeyStore} from '../src/time-key-store.mjs';
import {createMeetingRouter} from '../src/routes.mjs';
const admin='a'.repeat(40),device='d'.repeat(40),origin='https://meeting.example';
function publicKey(point){return createPublicKey({key:{kty:'EC',crv:'P-256',x:point.subarray(1,33).toString('base64url'),y:point.subarray(33,65).toString('base64url')},format:'jwk'})}
test('optional time endpoint signs fresh challenges without exposing keys or weakening other routes',async()=>{
 const store=new MeetingStore(':memory:');try{
  store.registerDevice('dev',device);const keys=new TimeKeyStore(store.db,{maxForwardSeconds:300});
  const pub=keys.initialize(origin,1800000000);let wall=1800000001,mono=100;
  const app=createMeetingRouter(store,{adminToken:admin,timeKeys:keys,timeSeconds:()=>wall,timeMonotonic:()=>mono,maxTimeProofPerMinute:2});
  const nonce='01'.repeat(32),r=await app.request('/time-proof/'+nonce);assert.equal(r.status,200);assert.equal(r.headers.get('cache-control'),'no-store');
  const b=await r.json();assert.deepEqual(Object.keys(b).sort(),['keyId','message','signature']);assert.equal(b.keyId,pub.keyId);
  const msg=Buffer.from(b.message,'hex');assert.equal(msg.subarray(4,36).toString('hex'),nonce);assert.equal(msg.readBigUInt64BE(68),BigInt(wall));assert.ok(verify('sha256',msg,publicKey(pub.publicKey),Buffer.from(b.signature,'hex')));
  assert.equal((await app.request('/meetings/private')).status,401);assert.equal((await app.request('/admin/time-signer',{headers:{authorization:`Bearer ${device}`}})).status,404);
  const a=await app.request('/admin/time-signer',{headers:{authorization:`Bearer ${admin}`}});assert.equal(a.status,200);const info=await a.json();assert.equal(info.publicKey,pub.publicKey.toString('hex'));assert.equal('privateKey' in info,false);
  assert.equal((await app.request('/time-proof/'+'00'.repeat(32))).status,400);assert.equal((await app.request('/time-proof/'+nonce+'?x=1')).status,400);
  assert.equal((await app.request('/time-proof/'+'02'.repeat(32))).status,200);assert.equal((await app.request('/time-proof/'+'03'.repeat(32))).status,429);
  mono+=60000;wall=1800000000;assert.equal((await app.request('/time-proof/'+'04'.repeat(32))).status,503);
  const disabled=createMeetingRouter(store,{adminToken:admin});assert.equal((await disabled.request('/time-proof/'+nonce)).status,404);
 }finally{store.close()}
});

test('only an authenticated administrator can provision the initial key and time',async()=>{
 const store=new MeetingStore(':memory:');try{
  store.registerDevice('dev',device);const keys=new TimeKeyStore(store.db,{maxForwardSeconds:300});
  const app=createMeetingRouter(store,{adminToken:admin,timeKeys:keys,timeSeconds:()=>1800000000});
  const request=(token,body)=>app.request('/admin/time-signer',{method:'POST',headers:{authorization:`Bearer ${token}`,'content-type':'application/json'},body:JSON.stringify(body)});
  assert.equal((await request(device,{origin,verifiedEpoch:1800000000})).status,404);
  assert.equal((await request(admin,{origin,verifiedEpoch:1799999900})).status,400);
  assert.equal((await request(admin,{origin:'http://meeting.example',verifiedEpoch:1800000000})).status,400);
  const r=await request(admin,{origin,verifiedEpoch:1800000000});assert.equal(r.status,200);const created=await r.json();
  const again=await request(admin,{origin,verifiedEpoch:1800000001});assert.equal(again.status,200);assert.deepEqual(await again.json(),created);
  assert.equal((await request(admin,{origin:'https://other.example',verifiedEpoch:1800000001})).status,409);
  assert.equal((await app.request('/time-proof/'+'01'.repeat(32))).status,200);
 }finally{store.close()}
});
