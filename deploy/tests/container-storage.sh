#!/usr/bin/env bash
set -euo pipefail
image=${1:?image required}
volume="meeting-ci-${GITHUB_RUN_ID:-local}-$$"
cleanup() { docker volume rm "$volume" >/dev/null 2>&1 || true; }
trap cleanup EXIT
docker volume create "$volume" >/dev/null
docker run --rm --user root -v "$volume:/data" "$image" sh -c 'chown 1000:1000 /data && chmod 700 /data'
run_node() { docker run --rm -i -v "$volume:/data" "$image" node --input-type=module; }
run_node <<'JS'
import {MeetingStore} from './src/store.mjs';
import {toOgg} from './src/audio.mjs';
import {createHash} from 'node:crypto';
const b=Buffer.alloc(100);b.write('SCR1');b.writeUInt32LE(1,20);b[96]=3;b.set([0xf8,0xff,0xfe],97);const audio=toOgg(b),hash=x=>createHash('sha256').update(x).digest('hex');
const s=new MeetingStore('/data/live.sqlite'),token='x'.repeat(40);s.registerDevice('d',token);for(const id of ['confirmed','deleted','pending']){s.create('d',id);s.put('d',id,0,audio,hash(audio));if(id!=='pending')s.finalize('d',id,{count:1,bytes:audio.length,digest:hash(audio),reason:'user'});}s.close();
JS
run_node <<'JS'
import {MeetingStore} from './src/store.mjs';import {validateAudioStructure} from './src/audio.mjs';import assert from 'node:assert/strict';const s=new MeetingStore('/data/live.sqlite');assert.equal(s.list('d','confirmed').committed,true);assert.equal(s.list('d','pending').chunks.length,1);validateAudioStructure(s.adminAudio('confirmed'));s.close();
JS
docker run --rm -v "$volume:/data" "$image" node src/backup-cli.mjs /data/live.sqlite /data/backup.sqlite
run_node <<'JS'
import {MeetingStore} from './src/store.mjs';import {createHash} from 'node:crypto';const s=new MeetingStore('/data/live.sqlite');s.adminDelete('deleted');s.revokeDevice('d',createHash('sha256').update('x'.repeat(40)).digest('hex'),'revoke-1');s.close();
JS
docker run --rm -v "$volume:/data" "$image" node src/restore-cli.mjs /data/backup.sqlite /data/live.sqlite /data/restored.sqlite
run_node <<'JS'
import {MeetingStore} from './src/store.mjs';import {validateAudioStructure} from './src/audio.mjs';import assert from 'node:assert/strict';const s=new MeetingStore('/data/restored.sqlite');assert.equal(s.authenticate('x'.repeat(40)),null);assert.throws(()=>s.adminGet('deleted'),/not_found/);assert.equal(s.list('d','pending').chunks.length,1);validateAudioStructure(s.adminAudio('confirmed'));s.close();console.log('container recreation and restore passed');
JS
