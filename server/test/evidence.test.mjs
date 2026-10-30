import { test } from 'node:test';
import { strict as assert } from 'node:assert';
import { validateEvidence } from '../src/evidence.mjs';
const versions = new Map([['v1', new Map([['s1', '会议😀确认方案']])]]);
test('validates Unicode ranges and exact text', () => {
  assert.equal(validateEvidence(versions, [{versionId:'v1',segmentId:'s1',start:2,end:5,quote:'😀确认'}]), true);
});
test('rejects missing, wrong-version, mismatched and invalid references', () => {
  assert.equal(validateEvidence(versions, []), false);
  for (const ref of [
    {versionId:'v2',segmentId:'s1',start:0,end:2,quote:'会议'},
    {versionId:'v1',segmentId:'s1',start:0,end:2,quote:'决定'},
    {versionId:'v1',segmentId:'s1',start:0,end:99,quote:'会议'},
  ]) assert.equal(validateEvidence(versions, [ref]), false);
});
test('classifies invalid references without returning original text',async()=>{const {evidenceError}=await import('../src/evidence.mjs');const ref={versionId:'v1',segmentId:'s1',start:0,end:2,quote:'会议'};for(const [references,code] of [[[],'invalid_evidence_missing'],[[null],'invalid_evidence_format'],[[{...ref,versionId:'wrong'}],'invalid_evidence_version'],[[{...ref,segmentId:'wrong'}],'invalid_evidence_segment'],[[{...ref,end:99}],'invalid_evidence_range'],[[{...ref,quote:'秘密'}],'invalid_evidence_quote']])assert.equal(evidenceError(versions,references),code);assert.equal(evidenceError(versions,[ref]),null)});
test('model quotes are located uniquely using Unicode offsets and never silently repair supplied ranges',async()=>{const {locateModelEvidence,evidenceError}=await import('../src/evidence.mjs');const transcript={versionId:'v1',segments:[{id:'s1',text:'会议😀确认方案'}]};const raw={discussion:[{text:'确认',evidence:[{versionId:'v1',segmentId:'s1',quote:'😀确认'}]}],decisions:[],actions:[]};const grounded=locateModelEvidence(raw,transcript);assert.equal(grounded.discussion[0].evidence[0].start,2);assert.equal(grounded.discussion[0].evidence[0].end,5);assert.equal(evidenceError(versions,grounded.discussion[0].evidence),null);const bad={...raw,discussion:[{text:'x',evidence:[{versionId:'v1',segmentId:'s1',quote:'😀确认',start:0,end:3}]}]};assert.equal(evidenceError(versions,locateModelEvidence(bad,transcript).discussion[0].evidence),'invalid_evidence_quote');assert.throws(()=>locateModelEvidence(raw,{versionId:'v1',segments:[{id:'s1',text:'😀确认😀确认'}]}),/invalid_evidence_ambiguous/);assert.throws(()=>locateModelEvidence(raw,{versionId:'v1',segments:[{id:'s1',text:'没有引用'}]}),/invalid_evidence_quote/)});
test('segment-only references derive immutable full segment without accepting incorrect supplied evidence',async()=>{const {locateModelEvidence,evidenceError}=await import('../src/evidence.mjs');const t={versionId:'v1',segments:[{id:'s1',text:'会议😀确认方案'}]};const entry={text:'确认',evidence:[{versionId:'v1',segmentId:'s1'}]},data={discussion:[entry],decisions:[],actions:[]};const ref=locateModelEvidence(data,t).discussion[0].evidence[0];assert.deepEqual(ref,{versionId:'v1',segmentId:'s1',start:0,end:7,quote:'会议😀确认方案'});assert.equal(evidenceError(versions,[ref]),null);assert.throws(()=>locateModelEvidence({...data,discussion:[{...entry,evidence:[{versionId:'v1',segmentId:'unknown'}]}]},t),/invalid_evidence_segment/);const wrong=locateModelEvidence({...data,discussion:[{...entry,evidence:[{versionId:'v1',segmentId:'s1',start:0,end:2,quote:'虚构'}]}]},t);assert.equal(evidenceError(versions,wrong.discussion[0].evidence),'invalid_evidence_quote')});
