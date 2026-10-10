#!/usr/bin/env node
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source=fs.readFileSync(__dirname+'/../downstream/wasm/web_net.c','utf8');
function body(name){const start=source.indexOf('EM_JS(',source.indexOf(name)-16);return source.slice(source.indexOf('{',start)+1,source.indexOf('\n});',start));}
const token='a'.repeat(64),instance='b'.repeat(32),storage=new Map();
const Module={roomListInstances:new Map([[0,instance],[1,'c'.repeat(32)]])};
const results=[],requests=[],allocated=new Set();let finish;
const context=vm.createContext({Module,UTF8ToString:x=>x,
 localStorage:{getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 stringToNewUTF8:x=>{allocated.add(x);return x},_free:x=>allocated.delete(x),_web_deleted_room:(id,error)=>results.push([id,error]),
 fetch:(url,options)=>{requests.push([url,options]);return new Promise(resolve=>finish=resolve);}});
vm.runInContext('function rawCan(address){'+body('Web_CanDeleteServer')+'}\nfunction remove(address){'+body('Web_DeleteServer')+'}\nfunction can(address){return !!rawCan(address);}',context);
(async()=>{
 assert.equal(context.can('127.0.0.1:28960'),false);
 storage.set('cod2-owned-room-0',JSON.stringify({instance,ownerToken:token}));Module.ownedRooms.clear();
 assert.equal(context.can('127.0.0.1:28960'),true);
 assert.equal(context.can('127.0.0.2:28960'),false);
 assert.equal(context.can('foreign:28960'),false);
 Module.roomListInstances.set(0,'c'.repeat(32));assert.equal(context.can('127.0.0.1:28960'),false);
 context.remove('127.0.0.1:28960');assert.equal(requests.length,0);assert.equal(results[0][0],-1);results.length=0;
 Module.roomListInstances.set(0,instance);context.remove('127.0.0.1:28960');context.remove('127.0.0.1:28960');
 assert.equal(requests.length,1);assert.equal(requests[0][0],'/servers');assert.equal(requests[0][1].method,'DELETE');
 assert.deepEqual(JSON.parse(requests[0][1].body),{id:0,ownerToken:token});
 finish({ok:false,json:async()=>({error:'Not owner'})});for(let i=0;i<12;i++)await Promise.resolve();
 assert.deepEqual(results,[[-1,'Not owner']]);assert.ok(Module.ownedRooms.has(0));assert.equal(Module.roomDeletionPending,false);results.length=0;
 context.remove('127.0.0.1:28960');finish({ok:true,json:async()=>({deleted:0})});for(let i=0;i<12;i++)await Promise.resolve();
 assert.deepEqual(results,[[0,0]]);assert.equal(Module.ownedRooms.has(0),false);assert.equal(storage.has('cod2-owned-room-0'),false);assert.equal(Module.roomDeletionPending,false);assert.equal(allocated.size,0);
 console.log('PASS: persisted ownership, foreign/reused room rejection, duplicate delete guard, error retention and successful deletion cleanup');
})().catch(error=>{console.error(error);process.exitCode=1;});
