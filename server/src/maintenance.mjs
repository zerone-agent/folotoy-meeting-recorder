/** Run housekeeping independently of long-running external model calls. */
export function startMaintenance(store,{retentionSeconds=null,intervalMs=3000,onError=()=>{}}={}){
 if(!Number.isSafeInteger(intervalMs)||intervalMs<1)throw new Error('invalid_interval');
 const tick=()=>{try{if(store.recoverDeletes().failed)onError('deletion_cleanup_failed')}catch{onError('deletion_cleanup_failed')}
  try{store.purgeShares?.()}catch{onError('share_cleanup_failed')}
  if(retentionSeconds!==null)try{store.expireCompleted(retentionSeconds)}catch{onError('retention_cleanup_failed')}};
 tick();const timer=setInterval(tick,intervalMs);return ()=>clearInterval(timer);
}
