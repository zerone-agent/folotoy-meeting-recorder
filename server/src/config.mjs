/** No automatic retention by default; explicitly configure positive whole days. */
export function retentionFromEnv(value){
 if(value===undefined||value==='')return null;
 if(!/^[1-9][0-9]{0,4}$/.test(value))throw new Error('MEETING_RETENTION_DAYS must be a positive integer');
 return Number(value)*86400;
}

export function storageLimitFromEnv(value){if(value===undefined||value==='')return null;if(!/^[1-9][0-9]*$/.test(value)||!Number.isSafeInteger(Number(value)))throw new Error('MEETING_STORAGE_LIMIT_BYTES must be a positive safe integer');return Number(value)}

export function portFromEnv(value){if(value===undefined)return 3443;if(!/^[1-9][0-9]{0,4}$/.test(value)||Number(value)>65535)throw new Error('MEETING_PORT must be an integer from 1 to 65535');return Number(value)}
export function requireToken(env,name,min=1){const value=env[name];if(typeof value!=='string'||value.length<min||/\s/.test(value))throw new Error(`${name} must be a non-whitespace credential`);return value}

export function workerConcurrencyFromEnv(value){if(value===undefined)return 2;if(!/^[1-4]$/.test(value))throw new Error('MEETING_WORKER_CONCURRENCY must be an integer from 1 to 4');return Number(value)}
