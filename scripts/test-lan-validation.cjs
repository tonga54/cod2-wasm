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
  const lan = await policy(undefined);
  assert.equal(lan.sha256, undefined, 'LAN uses the mandatory custom SHA-256 validator');
  assert.equal(lan.validateCached, undefined, 'framework must validate cached files too');
  await lan.validate(new Blob([bytes]));
  assert.equal(activeWorkers, 0);
  const corrupted = Buffer.from(bytes);
  corrupted[corrupted.length - 1] ^= 1;
  await assert.rejects(lan.validate(new Blob([corrupted])), /SHA-256 mismatch/);
  assert.equal(activeWorkers, 0, 'worker ends after rejection');
  const secure = await policy({subtle:{}});
  assert.equal(secure.sha256, hash, 'HTTPS keeps the framework Web Crypto validator');
  assert.equal(secure.validate, undefined);
  console.log('PASS: LAN validation accepts originals, rejects corruption, cleans workers, preserves HTTPS');
})().catch(error => { console.error(error); process.exitCode=1; });
