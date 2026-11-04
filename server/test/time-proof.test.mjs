import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash, generateKeyPairSync, verify} from 'node:crypto';
import {createTimeProofSigner,timeProofPublicKey} from '../src/time-proof.mjs';

test('MTP1 binds a P-256 signature to nonce, origin and UTC seconds',()=>{
 const {privateKey,publicKey}=generateKeyPairSync('ec',{namedCurve:'prime256v1'});
 const origin='https://meeting.example',nonce=Buffer.alloc(32,7),signer=createTimeProofSigner(privateKey,origin);
 assert.equal(timeProofPublicKey(privateKey).length,65);assert.equal(timeProofPublicKey(privateKey)[0],4);
 assert.deepEqual(timeProofPublicKey(publicKey),timeProofPublicKey(privateKey));
 const {message,signature}=signer(nonce,1800000000);
 assert.equal(message.length,76);assert.equal(message.subarray(0,4).toString(),'MTP1');
 assert.deepEqual(message.subarray(4,36),nonce);
 assert.deepEqual(message.subarray(36,68),createHash('sha256').update(origin).digest());
 assert.equal(message.readBigUInt64BE(68),1800000000n);
 assert.ok(verify('sha256',message,publicKey,signature));
 for(const offset of [0,4,36,75]){const altered=Buffer.from(message);altered[offset]^=1;assert.equal(verify('sha256',altered,publicKey,signature),false)}
 assert.throws(()=>signer(Buffer.alloc(32),1800000000));
 assert.throws(()=>signer(nonce,0));assert.throws(()=>signer(nonce,4102444801));
 assert.throws(()=>createTimeProofSigner(privateKey,'http://meeting.example'));
 assert.throws(()=>createTimeProofSigner(privateKey,'https://meeting.example/path'));
 const {privateKey:wrong}=generateKeyPairSync('ec',{namedCurve:'secp384r1'});
 assert.throws(()=>createTimeProofSigner(wrong,origin));
});
