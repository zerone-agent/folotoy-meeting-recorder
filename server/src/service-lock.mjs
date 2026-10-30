import {DatabaseSync} from 'node:sqlite';
import {openSync,closeSync,lstatSync,realpathSync} from 'node:fs';
import {dirname,basename,join} from 'node:path';

/** Shared service/restore exclusion lock; SQLite releases it on process exit. */
export function acquireServiceLock(databasePath){
 let canonical;try{canonical=realpathSync(databasePath)}catch(e){if(e.code!=='ENOENT')throw e;canonical=join(realpathSync(dirname(databasePath)),basename(databasePath))}const path=canonical+'.service-lock';
 try{const fd=openSync(path,'wx',0o600);closeSync(fd)}catch(e){if(e.code!=='EEXIST')throw e}
 const info=lstatSync(path);if(!info.isFile()||info.isSymbolicLink()||(info.mode&0o077)!==0||(typeof process.getuid==='function'&&info.uid!==process.getuid()))throw new Error('service lock must be a private regular file');
 let db;try{db=new DatabaseSync(path);db.exec('PRAGMA busy_timeout=0; BEGIN EXCLUSIVE');return {close(){db.exec('ROLLBACK');db.close()}}}catch(e){try{db?.close()}catch{}if(e.code==='ERR_SQLITE_ERROR'&&[5,6].includes(e.errcode&255))throw new Error('service_or_restore_running');throw e}
}
