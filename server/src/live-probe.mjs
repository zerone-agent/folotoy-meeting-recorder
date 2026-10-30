import { createInterface } from 'node:readline';
import { createRuntime } from '@zerone-agent/agent-runtime';
import { registerMinutesAgent } from './register-agent.mjs';
import { createRuntimeMinutes } from './runtime-minutes.mjs';
import { segmentTranscript,validateMinutes } from './minutes.mjs';
const lines=createInterface({input:process.stdin});const apiKey=(await new Promise(r=>lines.once('line',r))).trim();lines.close();if(!apiKey)throw new Error('missing key');const key='local-test-token-'+ 'x'.repeat(32);process.env.ZERONE_AGENT_HTTP_API_KEY=key;
const cfg={server:{host:'127.0.0.1',port:0},agents:[{id:'minutes',description:'probe',model:'step-3.7-flash',apiType:'openai-completions',apiKey,baseURL:'https://api.stepfun.com/step_plan/v1'}]};
let host;try{host=await createRuntime(cfg,{configDir:process.cwd()});registerMinutesAgent(host,{apiKey});await host.start();const transcript=segmentTranscript('张三建议先开发录音上传，小李建议下周验证，负责人和日期尚未确定。','synthetic-v1');const raw=await createRuntimeMinutes(host,key)(transcript,{signal:AbortSignal.timeout(90000)});const output=validateMinutes(raw,transcript,false,null);console.log('LIVE_RUNTIME_PASS',JSON.stringify({summary:!!output.summary,discussion:output.discussion.length,decisions:output.decisions.length,actions:output.actions.length}));}catch(e){const allowed=new Set(['invalid_evidence','invalid_minutes','minutes_invalid_json','minutes_incomplete','minutes_provider_failed']);console.log('LIVE_RUNTIME_FAIL',allowed.has(e?.message)?e.message:'request_or_validation_failed');process.exitCode=1}finally{if(host)await host.stop()}
