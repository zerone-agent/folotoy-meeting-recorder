import test from 'node:test';
import assert from 'node:assert/strict';
import {DatabaseSync} from 'node:sqlite';
import {TimeKeyStore} from '../src/time-key-store.mjs';
import {startTimeMaintenance} from '../src/time-maintenance.mjs';

test('checkpoint advances a configured floor and reports invalid clock without resetting it',()=>{
 const db=new DatabaseSync(':memory:');db.exec('CREATE TABLE service_metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL)');
 try{let mono=0,wall=1800000000;const keys=new TimeKeyStore(db,{maxForwardSeconds:300,monotonicSeconds:()=>mono}),errors=[];
  let stop=startTimeMaintenance(keys,{nowSeconds:()=>wall,onError:e=>errors.push(e)});stop();assert.equal(keys.getPublic(),null);
  keys.initialize('https://meeting.example',wall);mono+=3600;wall+=3600;
  stop=startTimeMaintenance(keys,{nowSeconds:()=>wall,onError:e=>errors.push(e)});stop();
  assert.equal(db.prepare('SELECT last_epoch FROM time_signer').get().last_epoch,wall);
  wall+=301;stop=startTimeMaintenance(keys,{nowSeconds:()=>wall,onError:e=>errors.push(e)});stop();
  assert.deepEqual(errors,['time_checkpoint_failed']);assert.equal(db.prepare('SELECT last_epoch FROM time_signer').get().last_epoch,1800003600);
 }finally{db.close()}
});
