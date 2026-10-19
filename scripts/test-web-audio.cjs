const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
let latest, decoders=[];
class Param {
  constructor(value){this.value=value;this.events=[];}
  setValueAtTime(v,t){this.value=v;this.events.push(['set',v,t]);}
  setTargetAtTime(v,t,c){this.events.push(['target',v,t,c]);}
}
class Node {
  constructor(kind){this.kind=kind;this.gain=new Param(1);this.pan=new Param(0);this.playbackRate=new Param(1);
    this.frequency=new Param(350);this.Q=new Param(1);this.outputs=[];}
  connect(node){this.outputs.push(node);return node;} disconnect(){this.disconnected=true;}
  start(t,offset){this.offset=offset;} stop(t){this.stopped=true;this.stopAt=t;}
}
class Context {
  constructor(){latest=this;this.currentTime=0;this.destination={};this.state='running';this.buffers=0;this.filters=[];}
  createGain(){return new Node('gain');} createStereoPanner(){return new Node('pan');}
  createBiquadFilter(){const node=new Node('biquad');this.filters.push(node);return node;}
  createBufferSource(){return this.source=new Node('source');}
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

  // An ordinary sample goes straight to its panner, without filters.
  const shot=audio.create();audio.pcm(shot,0,heap.length,48000,16,1,0);latest.currentTime=2;audio.play(shot);
  assert.deepEqual(latest.source.outputs.map(node=>node.kind),['pan']);assert.equal(latest.filters.length,0);
  assert(!latest.source.stopped);audio.stop(shot,true);
  // A suppressed one: the body highpassed at 140 Hz and lowpassed twice at 1.5 kHz, flat, a 3 kHz puff and a
  // 4.5 kHz click, each under its own envelope from the start, mixed into the panner and over in 0.32 s.
  audio.effect(shot,1);audio.reset(shot);audio.pcm(shot,0,heap.length,48000,16,1,0);latest.currentTime=3;audio.play(shot);
  const source=latest.source, branches=source.outputs;
  assert.deepEqual(branches.map(node=>[node.type,node.frequency.value,node.Q.value]),
                   [['highpass',140,-3.01],['bandpass',3000,1.2],['highpass',4500,-3.01]]);
  const lows=[branches[0].outputs[0],branches[0].outputs[0].outputs[0]];
  assert.deepEqual(lows.map(node=>[node.type,node.frequency.value,node.Q.value]),[['lowpass',1500,-3.01],['lowpass',1500,-3.01]]);
  const gains=[lows[1].outputs[0],branches[1].outputs[0],branches[2].outputs[0]];
  assert.deepEqual(gains.map(node=>node.gain.events),[
    [['set',1.5,3],['set',1.5,3.012],['target',0,3.012,0.05]],
    [['set',0.6,3],['set',0.6,3.002],['target',0,3.002,0.012]],
    [['set',0.4,3],['set',0.4,3.001],['target',0,3.001,0.004]]]);
  const mix=gains[0].outputs[0];
  assert(gains.every(node=>node.outputs.length===1&&node.outputs[0]===mix));
  assert.deepEqual(mix.outputs.map(node=>node.kind),['pan']);
  assert.equal(source.stopAt,3.32);assert.equal(audio.status(shot),4);
  // The filters go with the sample, whether it ends or is stopped.
  source.onended();assert.equal(audio.status(shot),2);
  assert(latest.filters.every(node=>node.disconnected)&&gains.every(node=>node.disconnected)&&mix.disconnected);
  audio.play(shot);const again=latest.source;audio.stop(shot,true);
  assert(again.outputs.flatMap(node=>[node,...node.outputs]).every(node=>node.disconnected));
  // A looping sample is never cut short; the effect stays until the engine sets another.
  audio.loop(shot,0);latest.currentTime=4;audio.play(shot);assert(!latest.source.stopped);audio.stop(shot,true);
  audio.effect(shot,0);audio.play(shot);assert.deepEqual(latest.source.outputs.map(node=>node.kind),['pan']);
  audio.shutdown();assert(latest.closed);
  console.log('PASS: PCM conversion/cache, pitch, pause/resume, looping, ending, async stream lifecycle and cleanup; '+
              'suppressed samples through their filters and envelopes, cut at 0.32 s, cleaned up when they end or stop');
})().catch(e=>{console.error(e);process.exitCode=1;});
