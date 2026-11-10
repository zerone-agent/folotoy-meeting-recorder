/** Render the meeting-minutes skill format; all model text is escaped. */
const clean=value=>typeof value==='string'&&value.trim()?value.replace(/[\r\n\t]+/g,' ').replace(/\\/g,'\\\\').replace(/([\`*_{}\[\]<>|#])/g,'\\$1').trim():'待确认';
const number=n=>{const digits=['零','一','二','三','四','五','六','七','八','九'];return n<10?digits[n]:n<20?'十'+(n%10?digits[n%10]:''):n<100?digits[Math.floor(n/10)]+'十'+(n%10?digits[n%10]:''):String(n)};
export function renderMinutesMarkdown(minutes,{captureReason,reviewed=false,includeReviewNotice=true}={}){
 if(!minutes||typeof minutes!=='object')throw new Error('invalid_minutes');
 const info=minutes.meeting&&typeof minutes.meeting==='object'?minutes.meeting:{},topic=clean(info.topic);
 const lines=[`# 会议纪要：${topic}`,'',`- **日期**：${clean(info.date)}`,`- **时长**：${clean(info.duration)}`,`- **形式**：${clean(info.format)}`,`- **参与方**：${clean(info.participants)}`,`- **主题**：${topic}`,'','---',''];
 if(minutes.incomplete || ['limit','storage','interrupted'].includes(captureReason))lines.push('> 录音不完整，纪要仅覆盖已保存内容。','');
 if(includeReviewNotice&&!reviewed)lines.push('> 自动转录和纪要未经人工校对；引用定位正确不代表结论一定得到原文支持。','');
 let section=0;const groups=[];for(const entry of Array.isArray(minutes.discussion)?minutes.discussion:[]){const title=clean(entry?.heading);let group=groups.find(g=>g.title===title);if(!group){group={title,items:[]};groups.push(group)}group.items.push(clean(entry?.text))}
 if(!groups.length&&typeof minutes.summary==='string'&&minutes.summary.trim())groups.push({title:'待确认',items:[clean(minutes.summary)]});
 for(const group of groups){lines.push(`## ${number(++section)}、${group.title==='待确认'?'议题':group.title}`,'',...group.items.map(x=>`- ${x}`),'')}
 const questions=Array.isArray(minutes.openQuestions)?minutes.openQuestions:[];if(questions.length)lines.push(`## ${number(++section)}、待确认事项`,'',...questions.map(x=>`- ${clean(x)}`),'');
 lines.push(`## ${number(++section)}、待跟进事项`,'','| 事项 | 负责方 | 优先级 |','|---|---|---|');const actions=Array.isArray(minutes.actions)?minutes.actions:[];for(const item of actions){const due=typeof item?.due==='string'&&item.due.trim()&&item.due.trim()!=='待确认'?`（期限：${clean(item.due)}）`:'';lines.push(`| ${clean(item?.text)}${due} | ${clean(item?.owner)} | ${clean(item?.priority)} |`)}if(!actions.length)lines.push('| 暂无明确事项 | 待确认 | 待确认 |');
 lines.push('',`## ${number(++section)}、关键结论`,'');const decisions=Array.isArray(minutes.decisions)?minutes.decisions:[];if(decisions.length)decisions.forEach((x,i)=>lines.push(`${i+1}. ${clean(x?.text)}`));else lines.push('暂无明确结论。');return lines.join('\n')+'\n';
}

// Identical user-authored text is preserved; its origin cannot be inferred.
export function containsReviewNotices(markdown){
 const notices=new Set(['自动转录和纪要未经人工校对；引用定位正确不代表结论一定得到原文支持。','当前纪要已人工复核。']);let fence=null;
 return markdown.split('\n').some(line=>{const match=/^ {0,3}(`{3,}|~{3,})/.exec(line);if(match){const marker=match[1];if(!fence)fence={char:marker[0],length:marker.length};else if(marker[0]===fence.char&&marker.length>=fence.length)fence=null;return false}if(fence||/^(?: {4}|\t)/.test(line))return false;return notices.has(line.trim().replace(/^>\s*/,''))});
}
export function minutesForDownload(markdown,reviewed){const ambiguous=containsReviewNotices(markdown);const status=reviewed?'当前纪要已人工复核。':'自动转录和纪要未经人工校对，请人工复核。';const note=ambiguous?'正文中包含复核状态相关文字，正文由管理员编辑，请以本提示的当前状态为准。':'';return (reviewed&&!ambiguous?'':'> '+status+(note?' '+note:'')+'\n\n')+markdown;}
