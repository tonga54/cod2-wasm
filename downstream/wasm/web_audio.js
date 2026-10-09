/* Browser output for the engine's Miles-compatible driver. No remote audio. */
function createCod2Audio(getHeap, log, Context = globalThis.AudioContext) {
  const context = new Context({ latencyHint: 'interactive' });
  const voices = new Map(), cache = new Map();
  let nextId = 1, cacheBytes = 0, starts = 0, failures = 0;
  const output = context.createGain();
  output.connect(context.destination);
  const debug = typeof location !== 'undefined' && new URLSearchParams(location.search).has('audioDebug');
  let analyser, meter, timer;
  if (debug) {
    analyser = context.createAnalyser();
    analyser.fftSize = 2048;
    output.connect(analyser);
    meter = document.createElement('output');
    meter.id = 'audio-debug';
    meter.style.cssText = 'position:fixed;bottom:0;left:0;z-index:99;background:#000c;color:white;font:12px monospace;padding:4px;pointer-events:none';
    document.body.append(meter);
    const values = new Float32Array(analyser.fftSize);
    timer = setInterval(() => {
      analyser.getFloatTimeDomainData(values);
      const rms = Math.sqrt(values.reduce((sum, x) => sum + x*x, 0) / values.length);
      meter.textContent = `Audio ${context.state} | starts ${starts} | active ${[...voices.values()].filter(v => v.status===4).length} | RMS ${rms.toFixed(5)} | errors ${failures} | cache ${(cacheBytes/1048576).toFixed(1)} MiB`;
    }, 250);
  }
  const unlock = () => { if (context.state === 'suspended') context.resume().catch(() => {}); };
  if (typeof document !== 'undefined') {
    document.addEventListener('pointerdown', unlock, true);
    document.addEventListener('keydown', unlock, true);
  }
  function remember(key, buffer) {
    if (cache.has(key)) return cache.get(key);
    const bytes = buffer.length * buffer.numberOfChannels * 4;
    while (cacheBytes + bytes > 32*1048576 && cache.size) {
      const [oldKey, old] = cache.entries().next().value;
      cache.delete(oldKey); cacheBytes -= old.length * old.numberOfChannels * 4;
    }
    if (bytes <= 32*1048576) { cache.set(key, buffer); cacheBytes += bytes; }
    return buffer;
  }
  function cached(key) {
    const buffer = cache.get(key);
    if (buffer) { cache.delete(key); cache.set(key, buffer); }
    return buffer;
  }
  function position(v) {
    let value = v.offset + (v.source ? (context.currentTime-v.started)*v.rate : 0);
    if (v.buffer && v.loop === 0) value %= v.buffer.duration;
    return Math.max(0, Math.min(value, v.buffer?.duration ?? value));
  }
  function stop(v, done) {
    if (!v) return;
    v.offset = done ? 0 : position(v);
    if (v.source) {
      v.source.onended = null;
      v.source.stop(); v.source.disconnect(); v.source = null;
    }
    v.gain?.disconnect(); v.panNode?.disconnect();
    v.gain = v.panNode = null;
    v.status = done ? 2 : 8;
  }
  function play(v) {
    if (!v || v.status === 4 && v.source) return;
    v.status = 4;
    if (!v.buffer) return; // An asynchronous stream decoder will start it.
    if (v.offset >= v.buffer.duration) {
      if (v.loop === 0) v.offset %= v.buffer.duration;
      else { v.status = 2; return; }
    }
    const source = context.createBufferSource();
    const gain = context.createGain(), pan = context.createStereoPanner();
    source.buffer = v.buffer; source.loop = v.loop === 0;
    source.playbackRate.value = v.rate;
    gain.gain.value = v.volume; pan.pan.value = v.pan;
    source.connect(pan); pan.connect(gain); gain.connect(output);
    v.source = source; v.gain = gain; v.panNode = pan; v.started = context.currentTime;
    source.onended = () => {
      if (v.source !== source) return;
      source.disconnect(); gain.disconnect(); pan.disconnect();
      v.source = v.gain = v.panNode = null;
      v.status = 2; v.offset = v.buffer.duration;
    };
    source.start(0, v.offset); starts++;
  }
  return {
    create() { const id = nextId++; voices.set(id, {status:2,offset:0,rate:1,volume:1,pan:0,loop:1,token:0}); return id; },
    reset(id) { const v=voices.get(id); if(!v)return; stop(v,true); v.token++; v.buffer=null; v.offset=0; v.rate=1; v.loop=1; },
    remove(id) { const v=voices.get(id); if(v){stop(v,true);v.token++;voices.delete(id);} },
    pcm(id, ptr, length, rate, bits, channels, floating) {
      const v=voices.get(id); if(!v)return;
      const key=`pcm:${ptr}:${length}:${rate}:${bits}:${channels}:${floating}`;
      let buffer=cached(key);
      if(!buffer) {
        const frames=Math.floor(length/(channels*bits/8));
        if(!frames || rate<3000 || rate>192000 || channels<1 || channels>2) return;
        buffer=context.createBuffer(channels,frames,rate);
        const bytes=getHeap(), view=new DataView(bytes.buffer,bytes.byteOffset+ptr,length);
        for(let channel=0;channel<channels;channel++) {
          const dest=buffer.getChannelData(channel);
          for(let i=0;i<frames;i++) {
            const p=(i*channels+channel)*(bits/8);
            if(bits===8) dest[i]=(view.getUint8(p)-128)/128;
            else if(bits===16) dest[i]=view.getInt16(p,true)/32768;
            else if(bits===24) dest[i]=((view.getUint8(p)|(view.getUint8(p+1)<<8)|(view.getInt8(p+2)<<16)))/8388608;
            else if(bits===32) dest[i]=floating ? view.getFloat32(p,true) : view.getInt32(p,true)/2147483648;
          }
        }
        remember(key,buffer);
      }
      v.buffer=buffer;
    },
    encoded(id, name, ptr, length) {
      const v=voices.get(id); if(!v)return;
      const key='file:'+name, hit=cached(key), token=++v.token;
      if(hit){v.buffer=hit;return;}
      const copy=getHeap().slice(ptr,ptr+length).buffer;
      context.decodeAudioData(copy).then(buffer => {
        if(!voices.has(id) || v.token!==token)return;
        v.buffer=remember(key,buffer);
        if(v.status===4)play(v);
      }).catch(error => {if(v.token===token){v.status=2;failures++;log('[audio] decode failed: '+name+' '+error.message);}});
    },
    play(id) { play(voices.get(id)); },
    stop(id, done) { stop(voices.get(id),done); },
    status(id) { return voices.get(id)?.status ?? 2; },
    time(id) { const v=voices.get(id);return v?position(v)*1000:0; },
    seek(id, ms) {const v=voices.get(id);if(!v)return;const playing=v.status===4;stop(v,false);v.offset=Math.max(0,ms/1000);if(playing)play(v);},
    rate(id, value) {const v=voices.get(id);if(!v||!(value>0))return;v.offset=position(v);v.started=context.currentTime;v.rate=value;if(v.source)v.source.playbackRate.setValueAtTime(value,context.currentTime);},
    levels(id, volume, pan) {const v=voices.get(id);if(!v)return;v.volume=Math.max(0,volume);v.pan=Math.max(-1,Math.min(1,pan));if(v.gain)v.gain.gain.setValueAtTime(v.volume,context.currentTime);if(v.panNode)v.panNode.pan.setValueAtTime(v.pan,context.currentTime);},
    loop(id, count) {const v=voices.get(id);if(v){v.loop=count;if(v.source)v.source.loop=count===0;}},
    clearCache() {cache.clear();cacheBytes=0;},
    shutdown() {for(const v of voices.values())stop(v,true);voices.clear();cache.clear();clearInterval(timer);meter?.remove();if(typeof document!=='undefined'){document.removeEventListener('pointerdown',unlock,true);document.removeEventListener('keydown',unlock,true);}context.close();}
  };
}
Module['cod2CreateAudio'] = createCod2Audio;
