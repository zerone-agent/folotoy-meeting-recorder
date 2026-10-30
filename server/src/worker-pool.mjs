import {processOne} from './worker.mjs';
/** Poll for work even while another worker is waiting on a model provider. */
export function startWorkerPool(store,adapters,{limit=2,intervalMs=3000,onError=()=>{}}={}){
 if(!Number.isSafeInteger(limit)||limit<1||limit>4||!Number.isSafeInteger(intervalMs)||intervalMs<1)throw new Error('invalid_worker_pool');
 const active=new Map();let stopped=false;
 const tick=()=>{if(stopped||adapters.shutdownSignal?.aborted)return;let jobs;try{jobs=store.ready()}catch{onError('worker_poll_failed');return}
  for(const job of jobs){if(active.size>=limit)break;if(active.has(job.id))continue;const stage=job.state==='asr_running'?'queued':job.state==='minutes_running'?'minutes_queued':job.state;const promise=processOne(store,job.id,stage,adapters).catch(()=>onError('worker_iteration_failed')).finally(()=>active.delete(job.id));active.set(job.id,promise)}
 };
 tick();const timer=setInterval(tick,intervalMs);
 return async()=>{stopped=true;clearInterval(timer);await Promise.allSettled(active.values())};
}
