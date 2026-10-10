// Explicit transport load benchmark: synthetic echo traffic, not game-player load.
import assert from 'node:assert/strict';
import {once} from 'node:events';
import dgram from 'node:dgram';
import {execFileSync} from 'node:child_process';
import {monitorEventLoopDelay, performance} from 'node:perf_hooks';
import WebSocket from 'ws';
import {createGateway} from './server.mjs';
const echo=dgram.createSocket('udp4');
echo.on('message',(data,peer)=>echo.send(data,peer.port,peer.address));
echo.bind(0,'0.0.0.0');await once(echo,'listening');
echo.setRecvBufferSize(2*1024*1024);
const gateway=createGateway({udpHost:'127.0.0.1',udpPort:echo.address().port});
gateway.server.listen(0,'127.0.0.1');await once(gateway.server,'listening');
let origin=`http://127.0.0.1:${gateway.server.address().port}`,dockerId;
const gatewayImage=process.env.BENCH_GATEWAY_IMAGE||'local/cod2-gateway:dev';
if(process.argv.includes('--docker')) {
 const entry="import {createGateway} from './server.mjs';const g=createGateway({udpHost:'host.docker.internal',udpPort:Number(process.env.UDP_PORT)});g.server.listen(8088,'0.0.0.0');";
 dockerId=execFileSync('docker',['run','--rm','-d','--cpus',process.env.BENCH_GATEWAY_CPUS||'2','--memory','96m','-p','127.0.0.1::8088','-e',`UDP_PORT=${echo.address().port}`,gatewayImage,'node','--input-type=module','-e',entry],{encoding:'utf8'}).trim();
 const mapped=execFileSync('docker',['port',dockerId,'8088/tcp'],{encoding:'utf8'}).trim();
 origin=`http://127.0.0.1:${mapped.split(':').at(-1)}`;
 for(let attempt=0;attempt<50;attempt++) {
  try {if((await fetch(origin+'/gateway/health')).ok)break;}catch{}
  await new Promise(resolve=>setTimeout(resolve,100));
 }
}
const clients=[],pending=new Map(),rtt=[],sizes=[128,1300];let sequence=0,received=0,sent=0,bad=0;
const lag=monitorEventLoopDelay({resolution:10});lag.enable();
try {
 for(let i=0;i<64;i++){
  const client=new WebSocket(origin.replace('http:','ws:')+'/game',{origin});
  clients.push(client);await once(client,'open');
  client.on('message',data=>{
   const id=data.readUInt32LE(0),request=pending.get(id);
   if(!request||request.client!==i||!data.equals(request.bytes)){bad++;return;}
   rtt.push(performance.now()-request.started);pending.delete(id);received++;
  });
 }
 // Each of 64 peers sends 60 datagrams/s for 10 s, using the real game's command/fragment payload sizes.
 const started=performance.now(),cpu=process.cpuUsage();let ticks=0;
 await new Promise(resolve=>{
  const timer=setInterval(()=>{
   for(let i=0;i<clients.length;i++){
    const id=sequence++,size=sizes[(ticks+i)%sizes.length],bytes=Buffer.alloc(size,id&255);bytes.writeUInt32LE(id,0);
    pending.set(id,{client:i,bytes,started:performance.now()});clients[i].send(bytes,{binary:true,compress:false});sent++;
   }
   if(++ticks===600){clearInterval(timer);resolve();}
  },1000/60);
 });
 const trafficSeconds=(performance.now()-started)/1000;
 for(let i=0;pending.size&&i<100;i++)await new Promise(resolve=>setTimeout(resolve,10));
 const used=process.cpuUsage(cpu);rtt.sort((a,b)=>a-b);
 const report={clients:64,synthetic:true,trafficSeconds,sent,received,lost:pending.size,bad,
  roundtripMs:{mean:rtt.reduce((a,b)=>a+b,0)/rtt.length,p95:rtt[Math.floor(rtt.length*.95)],p99:rtt[Math.floor(rtt.length*.99)],max:rtt.at(-1)},
  eventLoopMsP99:lag.percentile(99)/1e6,cpuPercentOneCore:(used.user+used.system)/trafficSeconds/10000,udpReceiveBufferBytes:echo.getRecvBufferSize(),gatewayEnvironment:dockerId?'Docker gateway, external clients/echo':'host process',
  gatewayImage:dockerId?gatewayImage:null,
  dockerCpuStat:dockerId?execFileSync('docker',['exec',dockerId,'cat','/sys/fs/cgroup/cpu.stat'],{encoding:'utf8'}):null};
 console.log('BENCHMARK '+JSON.stringify(report));assert.equal(pending.size,0);assert.equal(bad,0);
}finally{
 lag.disable();for(const client of clients)client.terminate();await gateway.close();echo.close();if(dockerId)execFileSync('docker',['stop',dockerId],{stdio:'ignore'});
}
