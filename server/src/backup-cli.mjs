import {backupDatabase} from './backup.mjs';
import {safeError} from './safe-error.mjs';
const [sourcePath,outputPath,...extra]=process.argv.slice(2);
if(!sourcePath||!outputPath||extra.length){console.error('Usage: node src/backup-cli.mjs SOURCE_DB NEW_BACKUP_DB');process.exitCode=2}
else{try{console.log(JSON.stringify(backupDatabase({sourcePath,outputPath})))}catch(error){console.error('Backup failed:',safeError(error));process.exitCode=1}}
