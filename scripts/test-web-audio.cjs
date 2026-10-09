const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
let latest, decoders=[];
class Node {
  constructor(){this.gain=this.pan=this.playbackRate={value:1,setValueAtTime(v){this.value=v;}};}
  connect(){} disconnect(){this.disconnected=true;} start(t,offset){this.offset=offset;} stop(){this.stopped=true;}
}
class Context {
  constructor(){latest=this;this.currentTime=0;this.destination={};this.state='running';this.buffers=0;}
  createGain(){return new Node();} createStereoPanner(){return new Node();}
  createBufferSource(){return this.source=new Node();}
  createBuffer(channels,length,rate){this.buffers++;const data=Array.from({length:channels},()=>new Float32Array(length));return {length,numberOfChannels:channels,duration:length/rate,getChannelData:i=>data[i]};}
  decodeAudioData(){return new Promise((resolve,reject)=>decoders.push({resolve,reject}));}
  close(){this.closed=true;} resume(){return Promise.resolve();}
}
const scope={Module:{},globalThis:{AudioContext:Context},DataView,Float32Array,Map,Math,setInterval,clearInterval};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../downstream/wasm/web_audio.js'),'utf8'),scope);
(async()=>{
  let heap=new Uint8Array(48000*2);const view=new DataView(heap.buffer);
  view.setInt16(0,-32768,true);view.setInt16(2,32767,true);
  const audio=scope.Module.cod2CreateAudio(()=>heap,()=>{}), id=audio.create();
  audio.pcm(id,0,heap.length,48000,16,1,0);audio.play(id);
  assert.equal(audio.status(id),4);assert.equal(latest.source.buffer.getChannelData(0)[0],-1);
  assert.equal(latest.source.buffer.getChannelData(0)[1],32767/32768);
  latest.currentTime=.25;assert.equal(audio.time(id),250);
  audio.rate(id,2);latest.currentTime=.5;assert.equal(audio.time(id),750);
  audio.stop(id,false);latest.currentTime=1;assert.equal(audio.time(id),750);assert.equal(audio.status(id),8);
  audio.play(id);assert.equal(latest.source.offset,.75);
  latest.source.onended();assert.equal(audio.status(id),2);
  audio.reset(id);audio.pcm(id,0,heap.length,48000,16,1,0);assert.equal(latest.buffers,1);
  audio.loop(id,0);audio.play(id);assert.equal(latest.source.loop,true);
  audio.stop(id,true);assert.equal(audio.time(id),0);
  audio.clearCache();audio.pcm(id,0,heap.length,48000,16,1,0);assert.equal(latest.buffers,2);
  // Delayed decoder must never resurrect a released or reused engine handle.
  const stream=audio.create();audio.encoded(stream,'music',0,10);audio.play(stream);audio.remove(stream);
  decoders.shift().resolve(latest.createBuffer(2,48000,48000));await Promise.resolve();
  assert.equal(audio.status(stream),2);
  const second=audio.create();audio.encoded(second,'ambient',0,10);audio.play(second);
  decoders.shift().resolve(latest.createBuffer(2,48000,48000));await Promise.resolve();
  assert.equal(audio.status(second),4);assert.equal(latest.source.buffer.numberOfChannels,2);
  audio.shutdown();assert(latest.closed);
  console.log('PASS: PCM conversion/cache, pitch, pause/resume, looping, ending, async stream lifecycle and cleanup');
})().catch(e=>{console.error(e);process.exitCode=1;});
