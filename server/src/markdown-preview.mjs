import MarkdownIt from 'markdown-it';
const markdown=new MarkdownIt({html:false,linkify:false,typographer:false});
const compact=t=>({type:t.type,tag:t.tag,nesting:t.nesting,content:t.content,children:t.children?.map(compact)});
export function renderPreview(source){if(typeof source!=='string')throw Error('invalid_markdown');return markdown.parse(source,{}).map(compact)}
