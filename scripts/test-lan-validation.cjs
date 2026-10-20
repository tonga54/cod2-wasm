const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const {createHash} = require('node:crypto');
const {sha256Blob} = require('../site/asset-sha256.js');
const adapterSource = fs.readFileSync(path.join(__dirname, '../site/native-game-adapter.js'), 'utf8');
const bytes = Buffer.from('PK\x03\x04original fixture');
const hash = createHash('sha256').update(bytes).digest('hex');
let activeWorkers = 0;
class TestWorker {
  constructor(url) { assert.equal(url, '/asset-sha256.js'); ++activeWorkers; }
  postMessage({file}) {
    sha256Blob(file).then(digest => this.onmessage({data: {digest}})).catch(() => this.onerror());
  }
  terminate() { --activeWorkers; }
}
async function policy(crypto) {
  const original = {name:'fixture.iwd', path:'main/fixture.iwd', size:bytes.length, magic:[80,75,3,4], sha256:hash};
  let files;
  const sandbox = {
    crypto, Worker:TestWorker, performance, URLSearchParams, location:{search:''},
    document:{addEventListener() {}, createElement:()=>({}), head:{appendChild() {}}},
    window:{addEventListener() {}}, addEventListener() {},
    fetch:async () => ({ok:true, json:async () => ({variants:{test:{files:[original]}}})})
  };
  vm.createContext(sandbox);
  vm.runInContext(adapterSource, sandbox);
  await sandbox.WasmGameAdapter.init({variant:'test', elements:{canvas:{addEventListener() {}}}, log() {},
    framework:{createOwnerDataSet(config) { files=config.files; return config; }}});
  assert.equal(original.sha256, hash, 'manifest remains intact');
  assert.equal(files[0].size, bytes.length);
  assert.deepEqual([...files[0].magic], [80,75,3,4]);
  return files[0];
}
(async () => {
  // HTTP LAN origins have no Web Crypto; HTTPS has it, but its digest needs a
  // whole archive in one buffer. Both hash downloads in the bounded worker.
  for (const crypto of [undefined, {subtle:{}}]) {
    const checked = await policy(crypto);
    assert.equal(checked.sha256, undefined, 'the framework never hashes a whole archive buffer');
    assert.equal(checked.validateCached, false,
      'a cached copy was hashed when stored; a start checks its size and signature only');
    await checked.validate(new Blob([bytes]));
    assert.equal(activeWorkers, 0);
    const corrupted = Buffer.from(bytes);
    corrupted[corrupted.length - 1] ^= 1;
    await assert.rejects(checked.validate(new Blob([corrupted])), /SHA-256 mismatch/);
    assert.equal(activeWorkers, 0, 'worker ends after rejection');
  }
  console.log('PASS: downloads on HTTP and HTTPS are hashed in the bounded worker, accept originals, reject ' +
    'corruption and clean up workers; cached copies skip the whole-archive hash');
})().catch(error => { console.error(error); process.exitCode=1; });
