import assert from 'node:assert/strict';
import http from 'node:http';
import dgram from 'node:dgram';
import { once } from 'node:events';
import WebSocket from 'ws';
import { createGateway } from './server.mjs';

// Endpoint/capacity contract only; the real dedicated and native UI are tested separately.
const echo = dgram.createSocket('udp4');
echo.on('message', (data, peer) => echo.send(data, peer.port, peer.address));
echo.bind(0, '127.0.0.1'); await once(echo, 'listening');
const udpPort = echo.address().port;
let creates = 0;
const manager = http.createServer(async (req, res) => {
  assert.equal(req.url, '/rooms');
  res.setHeader('Content-Type', 'application/json');
  if (req.method === 'POST') {
    let body=''; for await (const chunk of req) body += chunk;
    assert.deepEqual(JSON.parse(body), {name:'Second room'});
    ++creates; res.writeHead(201); res.end(JSON.stringify({id:1, port:udpPort+1}));
  } else res.end(JSON.stringify({rooms:[{id:0,port:udpPort},{id:1,port:udpPort+1}]}));
});
manager.listen(0,'127.0.0.1'); await once(manager,'listening');
const gateway = createGateway({udpHost:'127.0.0.1', udpPort, roomsEnabled:true,
  managerPort:manager.address().port});
gateway.server.listen(0,'127.0.0.1'); await once(gateway.server,'listening');
const origin = `http://127.0.0.1:${gateway.server.address().port}`;
const clients=[];
const deadline=setTimeout(()=>{throw new Error('Rooms test timed out');},8000);
async function open(room) {
  const client=new WebSocket(origin.replace('http:','ws:')+`/game?room=${room}`,{origin});
  clients.push(client); await once(client,'open'); return client;
}
async function rejected(room, expected) {
  await new Promise((resolve,reject)=>{
    const client=new WebSocket(origin.replace('http:','ws:')+`/game?room=${room}`,{origin});
    client.on('error',()=>{});
    client.on('open',()=>{client.terminate();reject(new Error('Unexpected upgrade'));});
    client.on('unexpected-response',(req,res)=>{
      try { assert.equal(res.statusCode,expected); res.resume(); req.destroy(); resolve(); }
      catch(error) {req.destroy();reject(error);}
    });
  });
}
try {
  let response=await fetch(origin+'/servers'); assert.equal(response.status,200);
  assert.equal((await response.json()).rooms.length,2);
  response=await fetch(origin+'/servers',{method:'POST',headers:{Origin:'http://unrelated.invalid'},body:'{}'});
  assert.equal(response.status,403); assert.equal(creates,0);
  response=await fetch(origin+'/servers',{method:'POST',headers:{Origin:origin},body:JSON.stringify({name:'Second room',command:'ignored'})});
  assert.equal(response.status,201); assert.equal(creates,1);
  const first=await open(0); await Promise.all(Array.from({length:63},()=>open(0)));
  response=await fetch(origin+'/servers');
  const occupied = (await response.json()).rooms;
  assert.equal(occupied.find(room=>room.id===0).connections,64);
  assert.equal(occupied.find(room=>room.id===0).maxPlayers,64);
  assert.equal(occupied.find(room=>room.id===1).connections,0);
  await rejected(0,503);
  await Promise.all(Array.from({length:64},()=>open(1))); await rejected(1,503);
  await rejected(2,503); await rejected('8',404);
  const reply=once(first,'message'); first.send(Buffer.from('room-zero'));
  assert.equal((await reply)[0].toString(),'room-zero');
  console.log('PASS: room listing/creation, same-origin writes, independent 64-slot limits, concurrent reservations, fixed destinations');
} finally {
  clearTimeout(deadline); for(const client of clients) client.terminate();
  await gateway.close(); await new Promise(resolve=>manager.close(resolve)); echo.close();
}
