const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync('downstream/wasm/web_net.c', 'utf8');
const body = source.match(/g_qport = EM_ASM_INT\(\{([\s\S]*?)\}\);/);
assert.ok(body, 'browser qport does not use the engine-relative startup clock');
const initialize = Function('globalThis', 'Math', body[1]);
const values = [0, 1, 256, 32768, 65535, 11741, 28003];
const crypto = {getRandomValues(buffer) {
  assert.ok(buffer instanceof Uint16Array && buffer.length === 1);
  buffer[0] = values.shift(); return buffer;
}};
for (const expected of [...values]) assert.equal(initialize({crypto}, Math), expected);
for (const random of [0, .1, .5, .999999999]) {
  const value = initialize({}, {floor:Math.floor, random:()=>random});
  assert.ok(Number.isInteger(value) && value >= 0 && value <= 65535);
}
const native = fs.readFileSync('src/PC/qcommon/net_chan_mp.c', 'utf8');
assert.match(native, /g_qport = \(unsigned short\)port;/,
  'native initialization still uses its supplied 16-bit port');
const startup = fs.readFileSync('src/PC/qcommon/common.c', 'utf8');
const netInit = startup.indexOf('    NET_Init();');
assert.ok(netInit > 0 && startup.indexOf('            CL_Init();', netInit) > netInit,
  'browser channel id is assigned before the client copies it');
console.log('PASS: independent browser channel ids, 16-bit boundaries and HTTP-compatible random fallback; native port path preserved');
