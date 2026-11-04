import {createHash, createPublicKey, sign} from 'node:crypto';

/** Encode the P-256 public point for authenticated device provisioning. */
export function timeProofPublicKey(key) {
 const pub=key?.type==='public'?key:createPublicKey(key);
 if(pub.asymmetricKeyType!=='ec'||pub.asymmetricKeyDetails?.namedCurve!=='prime256v1')throw new Error('invalid_time_key');
 const jwk=pub.export({format:'jwk'}),x=Buffer.from(jwk.x,'base64url'),y=Buffer.from(jwk.y,'base64url');
 if(x.length!==32||y.length!==32)throw new Error('invalid_time_key');
 return Buffer.concat([Buffer.from([4]),x,y]);
}

export function validateTimeProofOrigin(origin) {
 if(typeof origin!=='string')throw new Error('invalid_origin');
 let url;try{url=new URL(origin)}catch{throw new Error('invalid_origin')}
 if(url.protocol!=='https:'||url.origin!==origin||url.username||url.password)throw new Error('invalid_origin');
 return origin;
}

/** Candidate MTP1 signer. The private key and canonical HTTPS origin must be
 * provisioned and bound together outside this module. This module does not
 * publish an endpoint or manage key storage/rotation. */
export function createTimeProofSigner(privateKey, origin) {
 validateTimeProofOrigin(origin);
 timeProofPublicKey(privateKey);
 const originHash=createHash('sha256').update(origin,'utf8').digest();
 return (nonce,epoch)=>{
  if(!Buffer.isBuffer(nonce)||nonce.length!==32||nonce.every(x=>x===0)||!Number.isSafeInteger(epoch)||epoch<1735689600||epoch>4102444800)throw new Error('invalid_time_proof');
  const message=Buffer.alloc(76);message.write('MTP1');nonce.copy(message,4);originHash.copy(message,36);message.writeBigUInt64BE(BigInt(epoch),68);
  return {message,signature:sign('sha256',message,privateKey)};
 };
}
