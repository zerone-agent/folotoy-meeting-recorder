import { X509Certificate } from 'node:crypto';
import { createSecureContext } from 'node:tls';
export function validateTls(cert,key,now=Date.now()){
 const x=new X509Certificate(cert);if(now<Date.parse(x.validFrom)||now>Date.parse(x.validTo))throw new Error('TLS certificate is not currently valid');
 createSecureContext({cert,key});
}
export function waitForListening(server){return new Promise((resolve,reject)=>{const ready=()=>{server.off('error',failed);resolve()};const failed=e=>{server.off('listening',ready);reject(e)};server.once('listening',ready);server.once('error',failed)})}
