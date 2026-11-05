import {test} from 'node:test';
import {strict as assert} from 'node:assert';
import {runInNewContext} from 'node:vm';
import {meetingUi} from '../src/ui.mjs';
import {MeetingStore} from '../src/store.mjs';
import {createMeetingRouter} from '../src/routes.mjs';

class Element {
 constructor(){this.value='';this.textContent='';this.children=[];this.hidden=false;this.listeners={}}
 addEventListener(name,fn){this.listeners[name]=fn}
 append(...items){this.children.push(...items)}
 replaceChildren(...items){this.children=items}
 async click(){return this.listeners.click?.()}
 input(){return this.listeners.input?.()}
}
const key='meeting-recorder-admin-token-v1', ttl=7*24*60*60*1000;
function storage(){const values=new Map();return {getItem:k=>values.get(k)??null,setItem:(k,v)=>values.set(k,v),removeItem:k=>values.delete(k)}}
async function page(store,localStorage,clock){
 const html=await meetingUi().text(),script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
 const ids=Object.fromEntries(['token','status','list','detail','more','load','pending'].map(id=>[id,new Element()]));
 const app=createMeetingRouter(store,{adminToken:'meeting-recorder'});let requests=0;
 const context={document:{getElementById:id=>ids[id],createElement:()=>new Element()},location:{href:'https://local/meeting-service/ui'},URL,localStorage,Date:class extends Date{static now(){return clock.now}},setTimeout,confirm:()=>false,
  fetch:(url,options)=>{requests++;return app.request(new URL(url).pathname.replace('/meeting-service','')+new URL(url).search,options)}};
 runInNewContext(script,context);return {ids,requests:()=>requests};
}
const settle=()=>new Promise(resolve=>setTimeout(resolve,15));

test('verified token is restored across refresh and expires seven days after login',async()=>{
 const s=new MeetingStore(':memory:'),cache=storage(),clock={now:1800000000000};try{
  const first=await page(s,cache,clock);first.ids.token.value='meeting-recorder';await first.ids.load.click();
  const saved=JSON.parse(cache.getItem(key));assert.equal(saved.token,'meeting-recorder');assert.equal(saved.expiresAt,clock.now+ttl);
  clock.now+=1000;const second=await page(s,cache,clock);await settle();assert.equal(second.ids.token.value,'meeting-recorder');assert.ok(second.requests()>0);assert.equal(JSON.parse(cache.getItem(key)).expiresAt,saved.expiresAt);
  clock.now=saved.expiresAt;const calls=second.requests();await second.ids.load.click();assert.equal(second.ids.token.value,'');assert.equal(second.requests(),calls);assert.match(second.ids.status.textContent,/过期/);assert.equal(cache.getItem(key),null);
  cache.setItem(key,JSON.stringify(saved));const expired=await page(s,cache,clock);await settle();assert.equal(expired.ids.token.value,'');assert.equal(expired.requests(),0);assert.equal(cache.getItem(key),null);
 }finally{s.close()}
});

test('invalid credentials are not cached, and editing or server rejection clears a saved token',async()=>{
 const s=new MeetingStore(':memory:'),cache=storage(),clock={now:1800000000000};try{
  const first=await page(s,cache,clock);first.ids.token.value='bad-token';await first.ids.load.click();assert.equal(cache.getItem(key),null);
  first.ids.token.value='meeting-recorder';await first.ids.load.click();assert.ok(cache.getItem(key));first.ids.token.value='another-token';first.ids.token.input();assert.equal(cache.getItem(key),null);
  cache.setItem(key,JSON.stringify({token:'revoked-token',expiresAt:clock.now+ttl}));const rejected=await page(s,cache,clock);await settle();assert.equal(cache.getItem(key),null);assert.match(rejected.ids.status.textContent,/401/);
 }finally{s.close()}
});

test('malformed or unavailable browser storage never prevents manual use',async()=>{
 const s=new MeetingStore(':memory:'),cache=storage(),clock={now:1800000000000};try{
  cache.setItem(key,'bad json');const first=await page(s,cache,clock);assert.equal(cache.getItem(key),null);first.ids.token.value='meeting-recorder';await first.ids.load.click();assert.ok(cache.getItem(key));
  const blocked={getItem(){throw Error('blocked')},setItem(){throw Error('blocked')},removeItem(){throw Error('blocked')}};const second=await page(s,blocked,clock);second.ids.token.value='meeting-recorder';await second.ids.load.click();assert.equal(second.ids.status.textContent,'');
 }finally{s.close()}
});

test('admin token requires seven non-whitespace characters',()=>{
 const s=new MeetingStore(':memory:');try{
  for(const token of ['123456','bad key','       '])assert.throws(()=>createMeetingRouter(s,{adminToken:token}),/admin_token_required/);
  for(const token of ['1234567','meeting-recorder'])assert.doesNotThrow(()=>createMeetingRouter(s,{adminToken:token}));
 }finally{s.close()}
});
