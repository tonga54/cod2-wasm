import test from 'node:test';
import assert from 'node:assert/strict';
import { createMaintenanceCheck } from './maintenance.mjs';
import { createGateway } from './server.mjs';
import WebSocket from 'ws';
import { once } from 'node:events';

test('host gate validates expiry and tolerates missing/malformed files', () => {
  const now=()=>1000;
  for(const raw of ['{}','bad',JSON.stringify({until:999}),JSON.stringify({until:700000}), 'x'.repeat(1025)])
    assert.equal(createMaintenanceCheck({now,read:()=>raw})(),false);
  assert.equal(createMaintenanceCheck({now,read:()=>JSON.stringify({until:5000})})(),true);
  assert.equal(createMaintenanceCheck({now,read:()=>{throw Error('missing');}})(),false);
});

test('maintenance rejects new rooms and WebSockets while preserving health',async()=>{
  const gateway=createGateway({roomsEnabled:true,maintenanceCheck:()=>true});
  gateway.server.listen(0,'127.0.0.1');await once(gateway.server,'listening');
  const origin='http://127.0.0.1:'+gateway.server.address().port;
  try {
    const health=await (await fetch(origin+'/gateway/health')).json();
    assert.equal(health.updating,true);assert.equal(health.clients,0);assert.equal(health.pendingClients,0);
    assert.equal((await fetch(origin+'/servers',{method:'POST',headers:{Origin:origin},body:'{}'})).status,503);
    const socket=new WebSocket(origin.replace('http:','ws:')+'/game',{origin});
    const rejected=await new Promise(resolve=>{
      socket.on('unexpected-response',(_request,response)=>{resolve(response.statusCode);response.resume();socket.terminate();});
      socket.on('error',()=>{});
    });
    assert.equal(rejected,503);
  } finally { await new Promise(resolve=>gateway.server.close(resolve)); }
});
