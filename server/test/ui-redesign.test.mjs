import {test} from 'node:test';
import {strict as assert} from 'node:assert';
import {meetingUi} from '../src/ui.mjs';
test('workbench provides accessible search, filtering and session controls',async()=>{
 const response=meetingUi();const html=await response.text();
 assert.match(html,/id="search"/);assert.match(html,/id="filter"/);assert.match(html,/id="logout"/);assert.match(html,/id="workspace"/);
 assert.match(response.headers.get('content-security-policy'),/script-src 'sha256-/);
});
test('workbench favicon uses the Zerone mark and can be loaded under its service path',async()=>{const {MeetingStore}=await import('../src/store.mjs'),{createMeetingRouter}=await import('../src/routes.mjs');const store=new MeetingStore(':memory:');try{const html=await meetingUi().text();assert.match(html,/<link rel="icon" href="favicon\.svg" type="image\/svg\+xml">/);const res=await createMeetingRouter(store,{adminToken:'preview-test'}).request('/favicon.svg');assert.equal(res.status,200);assert.match(res.headers.get('content-type'),/^image\/svg\+xml/);assert.match(await res.text(),/aria-label="Zerone"/)}finally{store.close()}});
