Module.cod2P2P = (() => {
  const INFO_PEER = 0xffff;
  const QUEUE_PACKETS = 256;
  const QUEUE_BYTES = 524288;
  const BUFFER_LIMIT = 262144;
  const GATHER_MS = 1000;
  const INFO_QUERY = new Uint8Array([255, 255, 255, 255,
    ...Array.from('getinfo cod2-browser', character => character.charCodeAt(0))]);
  let link = null;
  let host = null;

  function signalUrl(query) {
    const url = new URL('/signal' + query, location.href);
    url.protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    return url;
  }

  function rtcConfig() {
    return { iceServers: [{ urls: 'stun:' + location.hostname + ':3478' }] };
  }

  function enqueue(state, item) {
    while (state.queue.length &&
           (state.queue.length >= QUEUE_PACKETS || state.bytes + item.packet.length > QUEUE_BYTES))
      state.bytes -= state.queue.shift().packet.length;
    state.queue.push(item);
    state.bytes += item.packet.length;
    Module.cod2BackgroundPump?.();
  }

  function dequeue(state, capacity) {
    while (state && state.queue.length) {
      const item = state.queue.shift();
      state.bytes -= item.packet.length;
      if (item.packet.length <= capacity) return item;
    }
    return null;
  }

  function sendJson(ws, value) {
    if (ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(value));
  }

  function parse(data) {
    try { return JSON.parse(data); } catch (_) { return null; }
  }

  function gathered(pc) {
    if (pc.iceGatheringState === 'complete') return Promise.resolve();
    return new Promise(resolve => {
      const done = () => {
        clearTimeout(timer);
        pc.removeEventListener('icegatheringstatechange', check);
        resolve();
      };
      const check = () => { if (pc.iceGatheringState === 'complete') done(); };
      const timer = setTimeout(done, GATHER_MS);
      pc.addEventListener('icegatheringstatechange', check);
    });
  }

  function trickle(pc, ws, fields) {
    let ready = false;
    pc.onicecandidate = event => {
      if (ready && event.candidate)
        sendJson(ws, Object.assign({ type: 'candidate', candidate: event.candidate.toJSON() }, fields));
    };
    return () => { ready = true; };
  }

  function failLink(state, message) {
    if (state.failed) return;
    state.failed = true;
    const text = new TextEncoder().encode('error\n' + message + '\0');
    const packet = new Uint8Array(4 + text.length);
    packet.fill(255, 0, 4);
    packet.set(text, 4);
    state.queue.length = 0;
    state.bytes = 0;
    enqueue(state, { packet });
  }

  async function offer(state) {
    if (typeof RTCPeerConnection !== 'function') return;
    try {
      const pc = new RTCPeerConnection(rtcConfig());
      state.pc = pc;
      const channel = pc.createDataChannel('cod2', { ordered: false, maxRetransmits: 0 });
      channel.binaryType = 'arraybuffer';
      channel.onopen = () => {
        if (state.stopped) return;
        state.channel = channel;
        out('[p2p] direct data channel to the host is open');
      };
      channel.onclose = () => {
        if (state.channel === channel) state.channel = null;
        if (!state.stopped && link === state && state.ws.readyState !== WebSocket.OPEN)
          failLink(state, 'EXE_SERVER_DISCONNECTED');
      };
      channel.onmessage = event => {
        if (!state.stopped && event.data instanceof ArrayBuffer)
          enqueue(state, { slot: state.slot, packet: new Uint8Array(event.data) });
      };
      const ready = trickle(pc, state.ws, {});
      await pc.setLocalDescription(await pc.createOffer());
      await gathered(pc);
      if (state.stopped) return;
      sendJson(state.ws, { type: 'offer', sdp: pc.localDescription.sdp });
      ready();
    } catch (error) {
      err('[p2p] direct link unavailable, using the gateway relay: ' + error.message);
    }
  }

  function openLink(slot) {
    if (link && link.slot === slot && !link.stopped && (!link.failed || link.queue.length)) return link;
    closeLink();
    const ws = new WebSocket(signalUrl('?room=' + slot));
    ws.binaryType = 'arraybuffer';
    const state = { slot, ws, pc: null, channel: null, queue: [], bytes: 0, sends: [], sendBytes: 0,
      stopped: false, opened: false, failed: false };
    link = state;
    ws.onopen = () => {
      if (state.stopped) return;
      state.opened = true;
      for (const packet of state.sends) ws.send(packet);
      state.sends.length = 0;
      state.sendBytes = 0;
      offer(state);
    };
    ws.onmessage = event => {
      if (state.stopped) return;
      if (event.data instanceof ArrayBuffer) {
        enqueue(state, { slot, packet: new Uint8Array(event.data) });
        return;
      }
      const message = parse(event.data);
      if (!message || !state.pc) return;
      if (message.type === 'answer' && typeof message.sdp === 'string')
        state.pc.setRemoteDescription({ type: 'answer', sdp: message.sdp }).catch(() => {});
      else if (message.type === 'candidate' && message.candidate)
        state.pc.addIceCandidate(message.candidate).catch(() => {});
    };
    ws.onclose = event => {
      if (state.stopped || link !== state) return;
      if (event.code === 4000) failLink(state, event.reason || 'EXE_SERVER_DISCONNECTED');
      else if (!state.opened) failLink(state, 'This game is no longer available. Choose another server from Join Game.');
      else if (!state.channel) failLink(state, 'EXE_SERVER_DISCONNECTED');
    };
    return state;
  }

  function closeLink() {
    const state = link;
    if (!state) return;
    link = null;
    state.stopped = true;
    state.queue.length = 0;
    state.bytes = 0;
    try { state.channel?.close(); } catch (_) {}
    try { state.pc?.close(); } catch (_) {}
    if (state.ws.readyState < 2) state.ws.close(1000);
  }

  function clientSend(slot, bytes) {
    const state = openLink(slot);
    if (state.failed) return;
    if (state.channel?.readyState === 'open') {
      if (state.channel.bufferedAmount < BUFFER_LIMIT) state.channel.send(bytes);
      return;
    }
    const ws = state.ws;
    if (ws.readyState === WebSocket.CONNECTING) {
      if (state.sends.length < 16 && state.sendBytes + bytes.length <= BUFFER_LIMIT) {
        state.sends.push(bytes.slice());
        state.sendBytes += bytes.length;
      }
      return;
    }
    if (ws.readyState === WebSocket.OPEN && ws.bufferedAmount < BUFFER_LIMIT) ws.send(bytes);
  }

  function clientNext(capacity) {
    return dequeue(link, capacity);
  }

  function dropPeer(state, id) {
    const peer = state.peers.get(id);
    if (!peer) return;
    state.peers.delete(id);
    try { peer.channel?.close(); } catch (_) {}
    try { peer.pc?.close(); } catch (_) {}
  }

  async function answer(state, id, sdp) {
    const peer = state.peers.get(id);
    if (!peer || peer.pc || typeof sdp !== 'string' || typeof RTCPeerConnection !== 'function') return;
    try {
      const pc = new RTCPeerConnection(rtcConfig());
      peer.pc = pc;
      pc.ondatachannel = event => {
        const channel = event.channel;
        channel.binaryType = 'arraybuffer';
        const attach = () => { if (state.peers.get(id) === peer) peer.channel = channel; };
        channel.onopen = attach;
        if (channel.readyState === 'open') attach();
        channel.onclose = () => { if (peer.channel === channel) peer.channel = null; };
        channel.onmessage = message => {
          if (!state.stopped && state.peers.get(id) === peer && message.data instanceof ArrayBuffer)
            enqueue(state, { peer: id, packet: new Uint8Array(message.data) });
        };
      };
      const ready = trickle(pc, state.ws, { peer: id });
      await pc.setRemoteDescription({ type: 'offer', sdp });
      await pc.setLocalDescription(await pc.createAnswer());
      await gathered(pc);
      if (state.stopped || state.peers.get(id) !== peer) { pc.close(); return; }
      sendJson(state.ws, { type: 'answer', peer: id, sdp: pc.localDescription.sdp });
      ready();
    } catch (error) {
      err('[p2p] direct link to a player failed, using the gateway relay: ' + error.message);
    }
  }

  function startTicker(state) {
    try {
      const url = URL.createObjectURL(new Blob(['setInterval(() => postMessage(0), 50);'],
        { type: 'text/javascript' }));
      const worker = new Worker(url);
      let last = 0;
      let painted = performance.now();
      let revoked = false;
      const watch = () => {
        if (state.stopped) return;
        painted = performance.now();
        requestAnimationFrame(watch);
      };
      requestAnimationFrame(watch);
      worker.onmessage = () => {
        if (!revoked) {
          revoked = true;
          URL.revokeObjectURL(url);
        }
        if (state.stopped) return;
        const now = performance.now();
        if (now - painted < 100 || now - last < 40) return;
        last = now;
        Module._web_background_frame();
      };
      state.ticker = worker;
    } catch (error) {
      err('[p2p] background host timer unavailable: ' + error.message);
    }
  }

  function tickHost(state) {
    if (state.stopped) return;
    const now = performance.now();
    if (Module._web_local_server_running()) {
      state.runningAt = now;
      if (now - state.infoAt >= 2000) {
        state.infoAt = now;
        enqueue(state, { peer: INFO_PEER, packet: INFO_QUERY });
      }
    } else {
      const limit = !state.runningAt ? 180000 : Module._web_client_connected() ? 60000 : 3000;
      if (now - (state.runningAt || state.startedAt) > limit) {
        stopHost();
        return;
      }
    }
    if (state.info && state.info !== state.infoSent && state.ws.readyState === WebSocket.OPEN) {
      state.infoSent = state.info;
      sendJson(state.ws, { type: 'info', info: state.info });
    }
  }

  function startHost() {
    stopHost();
    const ws = new WebSocket(signalUrl('?role=host'));
    ws.binaryType = 'arraybuffer';
    const now = performance.now();
    const state = { ws, slot: 0, peers: new Map(), queue: [], bytes: 0, stopped: false, info: '',
      infoSent: '', infoAt: 0, runningAt: 0, startedAt: now, timer: 0, ticker: null };
    host = state;
    ws.onmessage = event => {
      if (state.stopped) return;
      if (event.data instanceof ArrayBuffer) {
        const data = new Uint8Array(event.data);
        const id = data.length > 2 ? (data[0] << 8) | data[1] : 0;
        if (state.peers.has(id)) enqueue(state, { peer: id, packet: data.subarray(2) });
        return;
      }
      const message = parse(event.data);
      if (!message) return;
      if (message.type === 'hosted') out('[p2p] this browser hosts local room ' + message.slot);
      else if (message.type === 'join' && Number.isInteger(message.peer))
        state.peers.set(message.peer, { pc: null, channel: null });
      else if (message.type === 'offer') answer(state, message.peer, message.sdp);
      else if (message.type === 'candidate' && message.candidate)
        state.peers.get(message.peer)?.pc?.addIceCandidate(message.candidate).catch(() => {});
      else if (message.type === 'leave') dropPeer(state, message.peer);
    };
    ws.onclose = event => {
      if (!state.stopped) err('[p2p] lobby connection closed: ' + event.code + ' ' + event.reason);
    };
    state.timer = setInterval(() => tickHost(state), 1000);
    startTicker(state);
  }

  function stopHost() {
    const state = host;
    if (!state) return;
    host = null;
    state.stopped = true;
    clearInterval(state.timer);
    state.ticker?.terminate();
    for (const id of [...state.peers.keys()]) dropPeer(state, id);
    state.queue.length = 0;
    state.bytes = 0;
    if (state.ws.readyState < 2) state.ws.close(1000);
  }

  function hostSend(id, bytes) {
    const state = host;
    if (!state || state.stopped) return;
    if (id === INFO_PEER) {
      const text = String.fromCharCode.apply(null, bytes.subarray(4));
      const prefix = 'infoResponse\n';
      if (text.startsWith(prefix)) state.info = text.slice(prefix.length).split('\0', 1)[0].trim();
      return;
    }
    const peer = state.peers.get(id);
    if (!peer) return;
    if (peer.channel?.readyState === 'open') {
      if (peer.channel.bufferedAmount < BUFFER_LIMIT) peer.channel.send(bytes);
      return;
    }
    if (state.ws.readyState !== WebSocket.OPEN || state.ws.bufferedAmount >= BUFFER_LIMIT * 4) return;
    const frame = new Uint8Array(bytes.length + 2);
    frame[0] = id >> 8;
    frame[1] = id & 255;
    frame.set(bytes, 2);
    state.ws.send(frame);
  }

  function hostNext(capacity) {
    return dequeue(host, capacity);
  }

  function closeAll() {
    closeLink();
    stopHost();
  }

  return { clientSend, clientNext, closeLink, startHost, stopHost, hostSend, hostNext, closeAll };
})();
