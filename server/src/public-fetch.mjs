/** Keep runtime administration private; cap and drain public requests. */
export function createPublicFetch(handler,{isStopping=()=>false,maxInFlight=64}={}) {
 if(typeof handler!=='function'||!Number.isSafeInteger(maxInFlight)||maxInFlight<1)throw new Error('invalid_public_fetch');
 const active=new Set(),waiters=new Set(),shutdown=new AbortController();
 const publicFetch=async (request,...args)=>{
  const path=new URL(request.url).pathname;
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
