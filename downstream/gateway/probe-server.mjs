// Query the actual dedicated server through the LAN gateway. Not a gameplay test.
import assert from 'node:assert/strict';
import { once } from 'node:events';
import WebSocket from 'ws';

const origin = process.argv[2] || 'http://localhost:8088';
const url = new URL('/game', origin);
url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
const socket = new WebSocket(url, { origin });
const deadline = setTimeout(() => {
  socket.terminate();
  console.error('Dedicated-server query timed out');
  process.exitCode = 1;
}, 8000);
let repeat;
try {
  await once(socket, 'open');
  const result = once(socket, 'message');
  const query = Buffer.concat([Buffer.alloc(4, 255), Buffer.from('getinfo cod2-browser-check')]);
  socket.send(query);
  repeat = setInterval(() => socket.send(query), 500);
  const [message] = await result;
  assert(message.subarray(0, 4).equals(Buffer.alloc(4, 255)));
  const body = message.subarray(4).toString();
  assert(body.startsWith('infoResponse\n'), body);
  const tokens = body.split('\n')[1].replace(/\0+$/, '').split('\\').slice(1);
  const info = Object.fromEntries(Array.from({length: tokens.length / 2}, (_, i) => tokens.slice(i * 2, i * 2 + 2)));
  assert.equal(info.mapname, 'mp_toujane');
  assert.equal(info.gametype, 'tdm');
  assert.equal(info.sv_maxclients, '64');
  assert.equal(info.challenge, 'cod2-browser-check');
  console.log('PASS: actual server through WebSocket/UDP:', JSON.stringify(info));
} finally {
  clearTimeout(deadline);
  clearInterval(repeat);
  socket.close();
}
