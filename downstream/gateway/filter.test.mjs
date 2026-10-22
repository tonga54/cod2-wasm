import assert from 'node:assert/strict';
import { once } from 'node:events';
import dgram from 'node:dgram';
import test from 'node:test';
import WebSocket from 'ws';
import { createGateway, oobCommand, MAX_CLIENT_DATAGRAM, SERVER_BACKLOG_BYTES } from './server.mjs';

const oob = text => Buffer.concat([Buffer.from([255, 255, 255, 255]), Buffer.from(text, 'latin1')]);

test('the connectionless command is the word after 0xffffffff, as SV_ConnectionlessPacket reads it', () => {
  assert.equal(oobCommand(Buffer.from([1, 0, 0, 0, 255])), null, 'sequenced packets are not connectionless');
  assert.equal(oobCommand(Buffer.from([255, 255, 255])), null);
  assert.equal(oobCommand(oob('getchallenge')), 'getchallenge');
  assert.equal(oobCommand(oob('connect "\\name\\Soldier\\protocol\\118"')), 'connect');
  assert.equal(oobCommand(oob('getinfo xxx')), 'getinfo');
  assert.equal(oobCommand(oob('GetStatus\n')), 'getstatus');
  assert.equal(oobCommand(oob('RCON secret status')), 'rcon');
  assert.equal(oobCommand(oob('"rcon" secret status')), '"rcon"', 'quotes are not stripped into a command');
  assert.equal(oobCommand(oob(' rcon secret status')), '', 'leading whitespace yields no command');
  assert.equal(oobCommand(oob('')), '');
});

async function harness(options = {}) {
  const received = [];
  let peer = null;
  const game = dgram.createSocket('udp4');
  game.on('message', (bytes, source) => { received.push(bytes); peer = source; });
  game.bind(0, '127.0.0.1');
  await once(game, 'listening');
  const gateway = createGateway({ udpHost: '127.0.0.1', udpPort: game.address().port, ...options });
  gateway.server.listen(0, '127.0.0.1');
  await once(gateway.server, 'listening');
  const origin = `http://127.0.0.1:${gateway.server.address().port}`;
  const client = new WebSocket(origin.replace('http:', 'ws:') + '/game', { origin });
  await once(client, 'open');
  const settle = async count => {
    for (let i = 0; i < 100 && received.length < count; i++) await new Promise(resolve => setTimeout(resolve, 10));
    await new Promise(resolve => setTimeout(resolve, 50));
  };
  return { received, game, gateway, client, origin, settle, peer: () => peer,
    async close() { client.terminate(); await gateway.close(); game.close(); } };
}

test('only the commands a browser client sends reach the engine; rcon and the rest are dropped', {timeout: 5000}, async () => {
  const h = await harness();
  try {
    const allowed = [oob('getchallenge'), oob('connect "\\name\\Soldier"'), oob('getinfo xxx'), oob('getstatus'),
      Buffer.from([7, 0, 0, 0, 1, 2, 3])];
    const blocked = [oob('rcon secret status'), oob('RCON secret map_restart'), oob('"rcon" secret status'),
      oob(' rcon secret status'), oob('v 1234'), oob('ipAuthorize 1 accept'), oob('pb_sv_ver'),
      oob('getKeyAuthorize 0 key'), oob('disconnect'), oob(''), Buffer.from([255, 255, 255, 255])];
    for (const packet of [...blocked.slice(0, 5), ...allowed, ...blocked.slice(5)]) h.client.send(packet);
    await h.settle(allowed.length);
    assert.deepEqual(h.received, allowed);
    assert.equal(h.client.readyState, WebSocket.OPEN, 'a few refused commands do not disconnect the player');
  } finally { await h.close(); }
});

test('a browser that goes away tells the engine at once, from the port its player has', {timeout: 5000}, async () => {
  const h = await harness();
  try {
    h.client.send(oob('getchallenge'));
    await h.settle(1);
    const { port } = h.peer();
    h.client.close(1000);
    await h.settle(2);
    assert.deepEqual(h.received, [oob('getchallenge'), oob('disconnect')]);
    assert.equal(h.peer().port, port);
  } finally { await h.close(); }
  const late = await harness({ udpSocketFactory() {
    const udp = dgram.createSocket('udp4');
    const connect = udp.connect.bind(udp);
    udp.connect = (port, host, ready) => connect(port, host, () => setTimeout(ready, 100));
    return udp;
  } });
  try {
    late.client.send(oob('getchallenge'));
    late.client.close(1000);
    await new Promise(resolve => setTimeout(resolve, 250));
    assert.deepEqual(late.received, [], 'a socket closed before it was ready had nothing to leave');
  } finally { await late.close(); }
});

test('a datagram larger than any the client sends closes the connection', {timeout: 5000}, async () => {
  const h = await harness();
  try {
    h.client.send(Buffer.alloc(MAX_CLIENT_DATAGRAM, 1));
    await h.settle(1);
    assert.equal(h.received.length, 1, 'the largest legitimate size passes');
    const closed = once(h.client, 'close');
    h.client.send(Buffer.alloc(MAX_CLIENT_DATAGRAM + 1, 1));
    assert.equal((await closed)[0], 1009);
  } finally { await h.close(); }
});

test('packet, byte and connectionless budgets drop a flood; a sustained one disconnects', {timeout: 5000}, async () => {
  let clock = 0;
  const now = () => clock;
  const budget = { packets: [10, 20], bytes: [10000, 20000], oob: [1, 3], closeAfterDrops: 50, dropWindowMs: 10000 };
  let h = await harness({ clientBudget: budget, now });
  try {
    for (let i = 0; i < 30; i++) h.client.send(Buffer.from([i, 0, 0, 0]));
    await h.settle(20);
    assert.equal(h.received.length, 20, 'a burst beyond the packet budget is dropped');
    clock += 1000;
    for (let i = 0; i < 30; i++) h.client.send(Buffer.from([i, 1, 0, 0]));
    await h.settle(30);
    assert.equal(h.received.length, 30, 'the budget refills at its rate');
    clock += 10000;
    h.received.length = 0;
    for (let i = 0; i < 5; i++) h.client.send(oob('getstatus'));
    await h.settle(3);
    assert.equal(h.received.length, 3, 'connectionless commands have their own smaller budget');
    clock += 10000;
    h.received.length = 0;
    for (let i = 0; i < 3; i++) h.client.send(Buffer.alloc(MAX_CLIENT_DATAGRAM, i + 1));
    await h.settle(3);
    assert.equal(h.received.length, 3);
    for (let i = 0; i < 10; i++) h.client.send(Buffer.alloc(MAX_CLIENT_DATAGRAM, 9));
    await h.settle(10);
    assert.ok(h.received.length < 13, 'bytes beyond the byte budget are dropped');
    assert.equal(h.client.readyState, WebSocket.OPEN);
  } finally { await h.close(); }
  clock = 0;
  h = await harness({ clientBudget: budget, now });
  try {
    const closed = once(h.client, 'close');
    for (let i = 0; i < 100; i++) h.client.send(Buffer.from([i, 0, 0, 0]));
    assert.equal((await closed)[0], 1008, 'a client that keeps flooding is disconnected');
  } finally { await h.close(); }
});

test('snapshots behind a stalled connection are dropped beyond a bounded backlog', {timeout: 15000}, async () => {
  const h = await harness();
  try {
    h.client.send(Buffer.from([1, 0, 0, 0]));
    await h.settle(1);
    const [serverSide] = h.gateway.wss.clients;
    h.client._socket.pause();
    const snapshot = Buffer.alloc(1400, 7);
    let peak = 0;
    for (let batch = 0; batch < 120; batch++) {
      for (let i = 0; i < 100; i++) h.game.send(snapshot, h.peer().port, h.peer().address);
      await new Promise(resolve => setTimeout(resolve, 2));
      peak = Math.max(peak, serverSide.bufferedAmount);
    }
    assert.ok(peak >= SERVER_BACKLOG_BYTES / 2, `the stalled connection built a backlog (${peak} bytes)`);
    assert.ok(peak <= SERVER_BACKLOG_BYTES + 1500, `the backlog stays bounded (${peak} bytes)`);
  } finally { await h.close(); }
});

test('proxied pages refuse framing and keep referrers on this site', {timeout: 5000}, async () => {
  const http = await import('node:http');
  const web = http.createServer((request, response) => {
    response.writeHead(200, { 'Content-Type': 'text/html' });
    response.end('<!doctype html>');
  });
  web.listen(0, '127.0.0.1');
  await once(web, 'listening');
  const gateway = createGateway({ webHost: '127.0.0.1', webPort: web.address().port });
  gateway.server.listen(0, '127.0.0.1');
  await once(gateway.server, 'listening');
  try {
    const response = await fetch(`http://127.0.0.1:${gateway.server.address().port}/`);
    assert.equal(response.status, 200);
    assert.equal(response.headers.get('x-frame-options'), 'DENY');
    assert.equal(response.headers.get('referrer-policy'), 'same-origin');
    assert.equal(await response.text(), '<!doctype html>');
    assert.equal(gateway.server.headersTimeout, 15000);
    assert.equal(gateway.server.requestTimeout, 30000);
  } finally { await gateway.close(); web.close(); }
});
