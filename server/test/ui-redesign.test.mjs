import {test} from 'node:test';
import {strict as assert} from 'node:assert';
import {meetingUi} from '../src/ui.mjs';
test('workbench provides accessible search, filtering and session controls',async()=>{
 const response=meetingUi();const html=await response.text();
 assert.match(html,/id="search"/);assert.match(html,/id="filter"/);assert.match(html,/id="logout"/);assert.match(html,/id="workspace"/);
 assert.match(response.headers.get('content-security-policy'),/script-src 'sha256-/);
});
