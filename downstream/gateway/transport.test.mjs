import assert from 'node:assert/strict';
import { once } from 'node:events';
import dgram from 'node:dgram';
import WebSocket from 'ws';
import { createGateway } from './server.mjs';

// Transport isolation only; this is not a substitute for a real game test.
const timeout = setTimeout(() => { throw new Error('Gateway transport test timed out'); }, 8000);
const echo = dgram.createSocket('udp4');
const peers = new Set();
echo.on('message', (data, source) => {
  peers.add(source.port);
  echo.send(data, source.port, source.address);
});
echo.bind(0, '127.0.0.1');
await once(echo, 'listening');
const gateway = createGateway({ udpHost: '127.0.0.1', udpPort: echo.address().port });
gateway.server.listen(0, '127.0.0.1');
await once(gateway.server, 'listening');
const origin = `http://127.0.0.1:${gateway.server.address().port}`;
const url = origin.replace('http:', 'ws:') + '/game';
const clients = [];
try {
  for (let i = 0; i < 64; i++) {
    const client = new WebSocket(url, { origin });
    clients.push(client);
    await once(client, 'open');
  }
  const replies = clients.map(client => once(client, 'message'));
  const packets = clients.map((client, i) => Buffer.from([255,255,255,255,i,128,0]));
  clients.forEach((client, i) => client.send(packets[i]));
  const received = await Promise.all(replies);
  received.forEach((reply, i) => assert.deepEqual(reply[0], packets[i]));
  assert.equal(peers.size, 64, 'each client uses a distinct UDP source port');
  async function rejected(options, status) {
    await new Promise((resolve, reject) => {
      const client = new WebSocket(url, options);
      client.on('error', () => {});
      client.on('open', () => { client.terminate(); reject(new Error('Unexpected upgrade')); });
      client.on('unexpected-response', (request, response) => {
        try { assert.equal(response.statusCode, status); response.resume(); request.destroy(); resolve(); }
        catch (error) { request.destroy(); reject(error); }
      });
    });
  }
  await rejected({ origin }, 503);
  await rejected({ origin: 'http://unrelated.invalid' }, 403);
  console.log('PASS: binary UDP roundtrip, 64 isolated client ports, 64-slot limit, origin check');
} finally {
  for (const client of clients) client.terminate();
  await gateway.close();
  echo.close();
  clearTimeout(timeout);
}
