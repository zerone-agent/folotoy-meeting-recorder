/** Verify sensitive meeting metadata was removed while retaining the tombstone. */
export function tombstoneMetadataClean(db,id,state){
 const row=db.prepare('SELECT state,reason,chunks,bytes,digest,recorded_at,time_trust,completed_at,transcript,minutes,attempts,last_error,failed_stage,retry_at,lease_token,lease_until FROM meetings WHERE id=?').get(id);
 if(!row||row.state!==state||row.time_trust!=='uncalibrated'||row.attempts!==0||row.retry_at!==0)return false;
 return ['reason','chunks','bytes','digest','recorded_at','completed_at','transcript','minutes','last_error','failed_stage','lease_token','lease_until'].every(key=>row[key]===null);
}
