// Run inside a gateway container. Candidate checks include the WebSocket path;
// the fenced production check uses its private manager and UDP path.
import assert from 'node:assert/strict';
import dgram from 'node:dgram';
import WebSocket from 'ws';

const [revision, buildId, mode='candidate'] = process.argv.slice(2);
const base='http://127.0.0.1:8088';
const manager=mode==='production'?'http://cod2-server:8090/rooms':base+'/servers';
async function request(url,method='GET',body){
 const response=await fetch(url,{method,headers:{Origin:base,'Content-Type':'application/json'},
  body:body&&JSON.stringify(body),signal:AbortSignal.timeout(45000)});
 assert.ok(response.ok,`${method} ${new URL(url).pathname}: ${response.status}`);
 return response.json();
}
const build=await request(base+'/build-info.json');
assert.equal(build.revision,revision);assert.equal(build.buildId,buildId);assert.equal(build.dirty,false);
assert.equal((await request(base+'/gateway/health')).gateway,'ready');

async function challenge(room){
 if(mode==='production'){
  const socket=dgram.createSocket('udp4');
  try {
   await new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>reject(Error('Native UDP challenge timeout')),10000);
    socket.on('error',error=>{clearTimeout(timer);reject(error);});
    socket.on('message',message=>{
     if(message.subarray(4).toString().startsWith('challengeResponse')){clearTimeout(timer);resolve();}
    });
    socket.send(Buffer.concat([Buffer.alloc(4,255),Buffer.from('getchallenge')]),28960+room.id,'cod2-server');
   });
  }finally{socket.close();}
 }else{
  const socket=new WebSocket(base.replace('http:','ws:')+'/game?room='+room.id,{origin:base,perMessageDeflate:false});
  try {
   await new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>reject(Error('WebSocket native challenge timeout')),10000);
    socket.on('open',()=>socket.send(Buffer.concat([Buffer.alloc(4,255),Buffer.from('getchallenge')])));
    socket.on('error',error=>{clearTimeout(timer);reject(error);});
    socket.on('message',message=>{
     if(message.subarray(4).toString().startsWith('challengeResponse')){clearTimeout(timer);resolve();}
    });
   });
  }finally{socket.close();}
 }
}
for(const map of ['mp_toujane','mp_carentan','mp_dawnville','mp_burgundy','mp_railyard','mp_brecourt']){
 let room;
 try {
  room=await request(manager,'POST',{name:'Automatic update validation',map,gametype:'tdm',botCount:2,botDifficulty:'normal'});
  let ready=false;
  for(let i=0;i<30;i++){
   const observed=(await request(manager)).rooms.find(r=>r.id===room.id);
   if(observed?.bots===2&&observed.players===2){ready=true;break;}
   await new Promise(resolve=>setTimeout(resolve,1000));
  }
  assert.ok(ready,'Two living bots must join '+map);
  await challenge(room);
 }finally{
  if(room)await request(manager,'DELETE',{id:room.id,ownerToken:room.ownerToken});
 }
}
console.log(`PASS: ${mode} build metadata, six maps, two bots/map and native ${mode==='production'?'UDP':'WebSocket'} challenges`);
