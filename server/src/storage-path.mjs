import { mkdirSync,lstatSync } from 'node:fs';
import { join,resolve } from 'node:path';

/** Reject insecure pre-existing paths rather than silently changing permissions. */
export function prepareDataDirectory(input){
 const dir=resolve(input);mkdirSync(dir,{recursive:true,mode:0o700});
 const info=lstatSync(dir);if(!info.isDirectory()||info.isSymbolicLink())throw new Error('MEETING_DATA_DIR must be a real directory');
 if((info.mode&0o077)!==0)throw new Error('MEETING_DATA_DIR must not be accessible by group or others');
 if(typeof process.getuid==='function'&&info.uid!==process.getuid())throw new Error('MEETING_DATA_DIR must be owned by the service user');
 const db=join(dir,'meetings.sqlite');for(const path of [db,db+'-wal',db+'-shm',db+'-journal',db+'.service-lock',db+'.service-lock-journal'])try{const file=lstatSync(path);if(!file.isFile()||file.isSymbolicLink()||(file.mode&0o077)!==0)throw new Error('meeting database and sidecars must be private regular files');if(typeof process.getuid==='function'&&file.uid!==process.getuid())throw new Error('meeting database and sidecars must be owned by the service user')}catch(e){if(e.code!=='ENOENT')throw e}
 return {dir,db};
}
