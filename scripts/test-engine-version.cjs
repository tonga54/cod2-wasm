const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(require('node:path').join(__dirname, '../site/native-game-adapter.js'), 'utf8');
const start = source.indexOf('  function loadFactory() {');
const end = source.indexOf('  function startUpdateNotifier', start);
const locate = source.match(/locateFile: (path => `[^`]+`),/)[1];
(async () => {
  for (const mode of ['metadata', 'missing', 'offline']) {
    let script, calls = 0;
    const key = 'a'.repeat(64);
    const context = vm.createContext({
      AbortSignal, Date: { now: () => 1234 },
      fetch: async (url, options) => {
        calls++; assert.equal(url, '/build-info.json'); assert.equal(options.cache, 'no-store');
        if (mode === 'offline') throw new Error('offline');
        return { ok: true, json: async () => mode === 'metadata' ? {buildId: key} : {} };
      },
      document: { createElement: () => ({}), head: { appendChild: value => { script = value; context.createCod2Client = () => {}; value.onload(); } } }
    });
    vm.runInContext('let factoryPromise, moduleAssetQuery;\n' + source.slice(start, end) + `\nconst locateFile=${locate};`, context);
    const factory = await vm.runInContext('loadFactory()', context);
    assert.equal(await vm.runInContext('loadFactory()', context), factory);
    const suffix = '?build=' + (mode === 'metadata' ? key : '1234');
    assert.equal(script.src, '/cod2.js' + suffix);
    assert.equal(vm.runInContext('locateFile("cod2.wasm")', context), '/cod2.wasm' + suffix);
    assert.equal(calls, 1);
  }
  console.log('PASS: loader and WASM use one build URL, metadata bypasses cache, missing/offline fallback and a single factory');
})().catch(error => { console.error(error); process.exitCode = 1; });
