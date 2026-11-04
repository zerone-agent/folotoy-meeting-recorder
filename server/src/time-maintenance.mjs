/** Checkpoint the verified server wall clock while the service is running.
 * Never create a key, reset a floor, or hide an invalid/forward-jump clock. */
export function startTimeMaintenance(keys,{intervalMs=3600000,nowSeconds=()=>Math.floor(Date.now()/1000),onError=()=>{}}={}){
 if(!keys||!Number.isSafeInteger(intervalMs)||intervalMs<1||typeof nowSeconds!=='function'||typeof onError!=='function')throw Error('invalid_time_maintenance');
 const tick=()=>{try{keys.checkpoint(nowSeconds())}catch{onError('time_checkpoint_failed')}};
 tick();const timer=setInterval(tick,intervalMs);return ()=>clearInterval(timer);
}
