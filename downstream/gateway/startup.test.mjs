import assert from 'node:assert/strict';
import { once } from 'node:events';
import dgram from 'node:dgram';
import test from 'node:test';
import WebSocket from 'ws';
import { createGateway } from './server.mjs';

test('early gameplay packets survive delayed UDP readiness in order, within bounds', {timeout:5000}, async () => {
  const echo = dgram.createSocket('udp4');
  echo.on('message', (bytes, peer) => echo.send(bytes, peer.port, peer.address));
  echo.bind(0, '127.0.0.1');
  await once(echo, 'listening');
  const callbacks = new Set();
  const gateway = createGateway({udpHost:'127.0.0.1', udpPort:echo.address().port,
    udpSocketFactory() {
      const udp = dgram.createSocket('udp4');
      const connect = udp.connect.bind(udp), send = udp.send.bind(udp);
      udp.connect = (port, host, ready) => connect(port, host, () => setTimeout(ready, 75));
      udp.send = (bytes, callback) => { callbacks.add(callback); send(bytes, callback); };
      return udp;
    }});
  gateway.server.listen(0, '127.0.0.1');
  await once(gateway.server, 'listening');
  const origin = `http://127.0.0.1:${gateway.server.address().port}`;
  const clients = [];
  const open = async () => {
    const client = new WebSocket(origin.replace('http:', 'ws:') + '/game', {origin});
    clients.push(client); await once(client, 'open'); return client;
  };
  try {
    const client = await open();
    const sent = Array.from({length:16}, (_, i) => Buffer.from([255,255,255,255,i,128,0]));
    const received = [];
    const allReplies = new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('Initial packets were lost')), 800);
      client.on('message', bytes => {
        received.push(bytes);
        if (received.length === sent.length) { clearTimeout(timeout); resolve(); }
      });
    });
    for (const packet of sent) client.send(packet);
    await allReplies;
    assert.deepEqual(received, sent);
    assert.equal(callbacks.size, 1, 'the same send callback serves all peer commands');

    for (const size of [7, 4000, 40000]) {
      const flood = await open();
      const closed = once(flood, 'close');
      for (let i=0; i<17; i++) flood.send(Buffer.alloc(size, i));
      const [code] = await closed;
      assert.equal(code, 1009, 'startup packets and bytes are both bounded');
    }
    const abandoned = await open();
    const disconnected = once(abandoned, 'close');
    abandoned.send(Buffer.from([1])); abandoned.close(1000);
    await disconnected;
    // The delayed ready callback must tolerate an already closed UDP socket.
    await new Promise(resolve => setTimeout(resolve, 100));
  } finally {
    for (const client of clients) client.terminate();
    await gateway.close(); echo.close();
  }
});

test('a UDP send failure still closes the gameplay socket', {timeout:3000}, async () => {
  const gateway = createGateway({udpHost:'127.0.0.1',udpPort:28960,
    udpSocketFactory() {
      const udp=dgram.createSocket('udp4');
      udp.send=(_bytes, callback)=>setImmediate(()=>callback(new Error('Injected send failure')));
      return udp;
    }});
  gateway.server.listen(0,'127.0.0.1'); await once(gateway.server,'listening');
  const origin=`http://127.0.0.1:${gateway.server.address().port}`;
  const client=new WebSocket(origin.replace('http:','ws:')+'/game',{origin});
  try {
    await once(client,'open');
    const closed=once(client,'close');client.send(Buffer.from([255,255,255,255]));
    const [code]=await closed;assert.equal(code,1011);
  } finally { client.terminate();await gateway.close(); }
});
