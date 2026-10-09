const assert = require('node:assert/strict');
const {createHash} = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const {sha256Blob} = require('../site/asset-sha256.js');
const root = path.resolve(__dirname, '..');
async function check(bytes) {
  assert.equal(await sha256Blob(new Blob([bytes])), createHash('sha256').update(bytes).digest('hex'));
}
(async () => {
  assert.equal(await sha256Blob(new Blob([])), 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855');
  assert.equal(await sha256Blob(new Blob(['abc'])), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
  for (const length of [1,55,56,63,64,65,119,120,127,128,129,1048575,1048576,1048577]) {
    const bytes = Buffer.alloc(length);
    for (let i = 0; i < length; ++i) bytes[i] = (i * 71 + 13) & 255;
    await check(bytes);
  }
  const policy = JSON.parse(fs.readFileSync(path.join(root, 'site/wasm-game-data.json'))).variants['cod2-mp'];
  for (const file of policy.files) {
    const bytes = fs.readFileSync(path.join(root, 'data/browser', file.path));
    assert.equal(await sha256Blob(new Blob([bytes])), file.sha256, file.name);
    if (bytes.length < 10000) {
      bytes[bytes.length - 1] ^= 1;
      assert.notEqual(await sha256Blob(new Blob([bytes])), file.sha256, 'corruption must change the hash');
    }
  }
  console.log('PASS: LAN SHA-256 vectors, padding/chunk boundaries, original archives, corruption');
})().catch(error => { console.error(error); process.exitCode = 1; });
