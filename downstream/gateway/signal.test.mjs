import assert from 'node:assert/strict';
import http from 'node:http';
import dgram from 'node:dgram';
import { once } from 'node:events';
import WebSocket from 'ws';
import { attachStunResponder, createGateway } from './server.mjs';

const timeout = setTimeout(() => { throw new Error('Signal test timed out'); }, 8000);
const manager = http.createServer((request, response) => {
  response.setHeader('Content-Type', 'application/json');
  response.end(JSON.stringify({ rooms: [], maxRooms: 3 }));
});
manager.listen(0, '127.0.0.1');
await once(manager, 'listening');
const gateway = createGateway({ udpHost: '127.0.0.1', roomsEnabled: true, maxLocalRooms: 2,
  managerPort: manager.address().port });
gateway.server.listen(0, '127.0.0.1');
await once(gateway.server, 'listening');
const origin = `http://127.0.0.1:${gateway.server.address().port}`;
const sockets = [];
function connect(query, options = { origin }) {
  const socket = new WebSocket(`${origin.replace('http:', 'ws:')}/signal${query}`, options);
  sockets.push(socket);
  const inbox = [];
  const waiting = [];
  socket.on('message', (data, binary) => {
    const value = binary ? [...data] : JSON.parse(data);
    const next = waiting.shift();
    if (next) next(value); else inbox.push(value);
  });
  socket.next = () => inbox.length ? Promise.resolve(inbox.shift()) :
    new Promise(resolve => waiting.push(resolve));
  return socket;
}
async function rejected(query, status, options) {
  await new Promise((resolve, reject) => {
    const socket = new WebSocket(`${origin.replace('http:', 'ws:')}/signal${query}`, options || { origin });
    socket.on('error', () => {});
    socket.on('open', () => { socket.terminate(); reject(new Error('Unexpected upgrade')); });
    socket.on('unexpected-response', (request, response) => {
      try { assert.equal(response.statusCode, status); response.resume(); request.destroy(); resolve(); }
      catch (error) { request.destroy(); reject(error); }
    });
  });
}
const servers = async () => (await (await fetch(`${origin}/servers`)).json()).localRooms;
async function until(read, accept) {
  for (let attempt = 0; attempt < 100; ++attempt) {
    const value = await read();
    if (accept(value)) return value;
    await new Promise(resolve => setTimeout(resolve, 20));
  }
  throw new Error('Condition not reached');
}
const health = async () => (await fetch(`${origin}/gateway/health`)).json();
try {
  await rejected('?role=host', 403, { origin: 'http://unrelated.invalid' });
  const host = connect('?role=host');
  await once(host, 'open');
  assert.deepEqual(await host.next(), { type: 'hosted', slot: 1 });
  assert.deepEqual(await servers(), [], 'a room is listed only after its engine reports info');
  await rejected('?room=1', 404);

  host.send(JSON.stringify({ type: 'info', info: '\\clients\\2\\bots\\1\\hostname\\Patio\\mapname\\mp_toujane' }));
  const [listed] = await until(servers, rooms => rooms.length === 1);
  assert.equal(listed.slot, 1);
  assert.equal(listed.info, '\\clients\\2\\bots\\1\\hostname\\Patio [Local]\\mapname\\mp_toujane');
  assert.equal(listed.players, 2);
  assert.equal(listed.bots, 1);
  assert.equal(listed.connections, 1);

  const player = connect('?room=1');
  await once(player, 'open');
  assert.deepEqual(await host.next(), { type: 'join', peer: 1 });
  player.send(JSON.stringify({ type: 'offer', sdp: 'v=0 offer' }));
  assert.deepEqual(await host.next(), { type: 'offer', peer: 1, sdp: 'v=0 offer' });
  host.send(JSON.stringify({ type: 'answer', peer: 1, sdp: 'v=0 answer' }));
  assert.deepEqual(await player.next(), { type: 'answer', sdp: 'v=0 answer' });
  const candidate = { candidate: 'candidate:1 1 udp 1 10.42.0.20 5000 typ srflx', sdpMid: '0' };
  player.send(JSON.stringify({ type: 'candidate', candidate }));
  assert.deepEqual(await host.next(), { type: 'candidate', peer: 1, candidate });
  host.send(JSON.stringify({ type: 'candidate', peer: 1, candidate }));
  assert.deepEqual(await player.next(), { type: 'candidate', candidate });
  host.send(JSON.stringify({ type: 'answer', peer: 9, sdp: 'v=0 stray' }));

  player.send(Buffer.from([255, 255, 255, 255, 7]));
  assert.deepEqual(await host.next(), [0, 1, 255, 255, 255, 255, 7]);
  host.send(Buffer.from([0, 1, 9, 8, 7]));
  assert.deepEqual(await player.next(), [9, 8, 7]);
  assert.equal((await servers())[0].connections, 2);

  const second = connect('?room=1');
  await once(second, 'open');
  assert.deepEqual(await host.next(), { type: 'join', peer: 2 });
  second.close();
  assert.deepEqual(await host.next(), { type: 'leave', peer: 2 });

  const other = connect('?role=host');
  await once(other, 'open');
  assert.deepEqual(await other.next(), { type: 'hosted', slot: 2 });
  await rejected('?role=host', 503);
  other.close();
  await once(other, 'close');

  const status = await until(health, value => value.clients === 2);
  assert.equal(status.localRooms, 1);

  const [code, reason] = await Promise.all([once(player, 'close'), host.close()]).then(([event]) => event);
  assert.equal(code, 4000);
  assert.equal(String(reason), 'The host left the game.');
  await until(servers, rooms => rooms.length === 0);
  await rejected('?room=1', 404);

  const stun = attachStunResponder(dgram.createSocket('udp4'));
  stun.bind(0, '127.0.0.1');
  await once(stun, 'listening');
  const client = dgram.createSocket('udp4');
  client.bind(0, '127.0.0.1');
  await once(client, 'listening');
  const request = Buffer.alloc(20);
  request.writeUInt16BE(0x0001, 0);
  request.writeUInt32BE(0x2112a442, 4);
  request.fill(7, 8, 20);
  client.send(request, stun.address().port, '127.0.0.1');
  const [reply] = await once(client, 'message');
  assert.equal(reply.readUInt16BE(0), 0x0101);
  assert.deepEqual(reply.subarray(8, 20), request.subarray(8, 20));
  assert.equal(reply.readUInt16BE(20), 0x0020);
  assert.equal(reply.readUInt16BE(26) ^ 0x2112, client.address().port);
  assert.deepEqual([...reply.subarray(28, 32)].map((octet, i) => octet ^ [0x21, 0x12, 0xa4, 0x42][i]), [127, 0, 0, 1]);
  client.close();
  stun.close();
  console.log('PASS: browser-hosted room listing, signaling relay, datagram relay, departures, limits and LAN STUN');
} finally {
  for (const socket of sockets) socket.terminate();
  await gateway.close();
  manager.close();
  clearTimeout(timeout);
}
