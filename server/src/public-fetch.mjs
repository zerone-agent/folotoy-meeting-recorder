import {readFileSync} from 'node:fs';
const packageVersion=JSON.parse(readFileSync(new URL('../package.json',import.meta.url),'utf8')).version;
/** Keep runtime administration private; cap and drain public requests. */
export function createPublicFetch(handler,{isStopping=()=>false,maxInFlight=64,version=process.env.MEETING_VERSION||packageVersion}={}) {
 if(typeof handler!=='function'||!Number.isSafeInteger(maxInFlight)||maxInFlight<1)throw new Error('invalid_public_fetch');
 const active=new Set(),waiters=new Set(),shutdown=new AbortController();
 const publicFetch=async (request,...args)=>{
  const path=new URL(request.url).pathname;
  if(path==='/health'){if(isStopping()||shutdown.signal.aborted)return Response.json({status:'stopping'},{status:503,headers:{'cache-control':'no-store'}});if(request.method!=='GET'&&request.method!=='HEAD')return new Response(null,{status:405,headers:{allow:'GET, HEAD','cache-control':'no-store'}});return request.method==='HEAD'?new Response(null,{status:200,headers:{'cache-control':'no-store'}}):Response.json({status:'ok',version},{headers:{'cache-control':'no-store'}})}
  if(path!=='/meeting-service'&&!path.startsWith('/meeting-service/'))return Response.json({error:'not_found'},{status:404,headers:{'cache-control':'no-store'}});
  if(isStopping()||shutdown.signal.aborted)return Response.json({error:'shutting_down'},{status:503,headers:{'cache-control':'no-store','retry-after':'5'}});
  if(active.size>=maxInFlight)return Response.json({error:'busy'},{status:429,headers:{'cache-control':'no-store','retry-after':'1'}});
  const marker={};active.add(marker);
  try{const wrapped=new Request(request,{signal:AbortSignal.any([request.signal,shutdown.signal])});return await handler(wrapped,...args)}
  finally{active.delete(marker);if(active.size===0){for(const done of waiters)done();waiters.clear()}}
 };
 publicFetch.abortAll=()=>shutdown.abort();
 publicFetch.waitForIdle=()=>active.size===0?Promise.resolve():new Promise(resolve=>waiters.add(resolve));
 return publicFetch;
}
