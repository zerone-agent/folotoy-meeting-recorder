import {createServer} from 'node:http';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {MeetingStore} from '../../../server/src/store.mjs';
import {TimeKeyStore} from '../../../server/src/time-key-store.mjs';
import {createMeetingRouter} from '../../../server/src/routes.mjs';
const dir=process.argv[2],store=new MeetingStore(':memory:'),keys=new TimeKeyStore(store.db,{maxForwardSeconds:300});
const point=keys.initialize(process.env.MEETING_TEST_ORIGIN||'https://127.0.0.1',1799999999).publicKey;
const app=createMeetingRouter(store,{adminToken:'a'.repeat(40),timeKeys:keys,timeSeconds:()=>1800000000});
const server=createServer(async(req,res)=>{
 try {
  if(req.headers.authorization||req.method!=='GET'||!req.url.startsWith('/meeting-service/time-proof/')){res.writeHead(400);res.end();return}
  if(process.env.MEETING_TEST_DELAY_MS)await new Promise(resolve=>setTimeout(resolve,Number(process.env.MEETING_TEST_DELAY_MS)));
  const response=await app.request(req.url.slice('/meeting-service'.length));
  const body=Buffer.from(await response.arrayBuffer());res.writeHead(response.status,{'content-type':'application/json','content-length':body.length,'connection':'close'});res.end(body);
 } catch {res.writeHead(500);res.end()}
});
server.listen(0,process.env.MEETING_TEST_BIND||'127.0.0.1',()=>{writeFileSync(join(dir,'live-point.bin'),point);writeFileSync(join(dir,'live-port.txt'),String(server.address().port))});
process.on('SIGTERM',()=>server.close(()=>{store.close();process.exit(0)}));
