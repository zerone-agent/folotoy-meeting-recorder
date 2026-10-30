import { restoreDatabase } from './restore.mjs';
import {safeError} from './safe-error.mjs';
const [backupPath,safetyPath,outputPath,...extra]=process.argv.slice(2);
if(!backupPath||!safetyPath||!outputPath||extra.length){console.error('Usage: node src/restore-cli.mjs BACKUP_DB CURRENT_SAFETY_DB NEW_OUTPUT_DB');process.exitCode=2}
else {try{const result=restoreDatabase({backupPath,safetyPath,outputPath});console.log(JSON.stringify(result));}catch(e){console.error(`Restore failed: ${safeError(e)}`);process.exitCode=1}}
