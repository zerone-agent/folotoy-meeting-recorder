import {lstatSync} from 'node:fs';
/** New SQLite outputs must not be paired with pre-existing journals. */
export function rejectExistingSidecars(path){for(const suffix of ['-wal','-shm','-journal']){try{lstatSync(path+suffix)}catch(e){if(e.code==='ENOENT')continue;throw e}throw new Error('output_sidecar_exists')}}
