/** Return a content-free error category; offsets use Unicode code points. */
export function evidenceError(versions,references){
 if(!Array.isArray(references)||references.length===0)return 'invalid_evidence_missing';
 for(const ref of references){
  if(!ref||typeof ref!=='object')return 'invalid_evidence_format';
  const {versionId,segmentId,start,end,quote}=ref,version=versions.get(versionId);
  if(!version)return 'invalid_evidence_version';
  const segment=version.get(segmentId);if(typeof segment!=='string')return 'invalid_evidence_segment';
  if(typeof quote!=='string'||!Number.isInteger(start)||!Number.isInteger(end))return 'invalid_evidence_format';
  const points=Array.from(segment);if(start<0||start>=end||end>points.length)return 'invalid_evidence_range';
  if(points.slice(start,end).join('')!==quote)return 'invalid_evidence_quote';
 }
 return null;
}
export function validateEvidence(versions,references){return evidenceError(versions,references)===null}
/** Attach exact source text for a selected segment or locate a unique quotation. */
export function locateModelEvidence(raw,transcript){
 if(!raw||typeof raw!=='object')return raw;
 const segments=new Map(transcript.segments.map(s=>[s.id,s.text]));const result={...raw};
 for(const field of ['discussion','decisions','actions']){
  if(!Array.isArray(raw[field]))continue;
  result[field]=raw[field].map(entry=>{
   if(!entry||typeof entry!=='object'||!Array.isArray(entry.evidence))return entry;
   return {...entry,evidence:entry.evidence.map(ref=>{
    if(!ref||typeof ref!=='object'||ref.start!==undefined||ref.end!==undefined)return ref;
    if(ref.versionId!==transcript.versionId)throw new Error('invalid_evidence_version');
    const segment=segments.get(ref.segmentId);if(typeof segment!=='string')throw new Error('invalid_evidence_segment');
    if(ref.quote===undefined){if(!segment.length)throw new Error('invalid_evidence_format');return {...ref,start:0,end:Array.from(segment).length,quote:segment};}
    if(typeof ref.quote!=='string'||!ref.quote.length)throw new Error('invalid_evidence_format');
    const first=segment.indexOf(ref.quote);if(first<0)throw new Error('invalid_evidence_quote');
    if(segment.indexOf(ref.quote,first+1)!==-1)throw new Error('invalid_evidence_ambiguous');
    const start=Array.from(segment.slice(0,first)).length;return {...ref,start,end:start+Array.from(ref.quote).length};
   })};
  });
 }
 return result;
}
