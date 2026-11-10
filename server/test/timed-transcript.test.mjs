import {test} from 'node:test';import {strict as assert} from 'node:assert';import {segmentTimedTranscript} from '../src/minutes.mjs';
test('timed transcript keeps provider segments and rejects invalid timing without inventing offsets',()=>{
 const t=segmentTimedTranscript([{text:'第一句。',words:[{start:1,end:2},{start:2,end:3}]},{text:'第二句。',words:[{start:8,end:7}]}],'v1');
 assert.equal(t.segments[0].startSeconds,1);assert.equal(t.segments[0].endSeconds,3);assert.equal(t.segments[1].startSeconds,undefined);assert.deepEqual(t.segments.map(s=>s.id),['s0','s1']);
});
test('approximate stream never replaces the independently recognized full text',()=>{
 const parts=[{text:'这是相似的片段。',startSeconds:0,endSeconds:2,comparison:'approximate'}];parts.fullText='这是相似的片段，以及识别出的尾部。';const transcript=segmentTimedTranscript(parts,'v2');
 assert.equal(transcript.segments.map(x=>x.text).join(''),parts.fullText);assert.equal(transcript.timedAlternatives[0].text,parts[0].text);assert.equal(transcript.timedAlternatives[0].startSeconds,0);
});
