import {test} from 'node:test';import {strict as assert} from 'node:assert';import {renderPreview} from '../src/markdown-preview.mjs';
test('renders common meeting markdown without executing HTML or unsafe links',()=>{
 const tokens=renderPreview('# 纪要\n\n> 提醒\n\n|事项|负责人|\n|---|---|\n|测试|小王|\n\n1. 一项\n\n<script>alert(1)</script> [bad](javascript:alert(1))');
 assert.ok(tokens.some(t=>t.tag==='h1'));assert.ok(tokens.some(t=>t.tag==='blockquote'));assert.ok(tokens.some(t=>t.tag==='table'));assert.ok(tokens.some(t=>t.tag==='ol'));assert.ok(tokens.some(t=>t.children?.some(c=>c.content.includes('<script>'))));
});
