const assert=require('node:assert/strict');
const fs=require('node:fs');
const {prepareWebShell}=require('./prepare-web-shell.cjs');
const framework=process.env.COD2_WASM_FRAMEWORK_DIR||'../wasm-game-framework';
const original=fs.readFileSync(framework+'/dist/index.html','utf8');
const result=prepareWebShell(original);
const gate=result.indexOf('id="cod2-startup-gate"');
assert.ok(gate>0&&gate<result.indexOf('</head>')&&gate<result.indexOf('<body>'));
assert.ok(result.indexOf('href="/startup.css"')<result.indexOf('<body>'));
for(const weight of ['Medium','Regular']){
  const preload=result.indexOf('<link rel="preload" href="/fonts/Barlow-'+weight+'.ttf" as="font" type="font/ttf" crossorigin>');
  assert.ok(preload>gate&&preload<result.indexOf('</head>'));
}
const head=result.slice(0,result.indexOf('</head>'));
assert.ok(head.includes('<link rel="manifest" href="/app.webmanifest" crossorigin="use-credentials">'),
  'a host\'s login cookie reaches the manifest');
assert.ok(head.includes('<link rel="apple-touch-icon" href="/game-data/files/cod2-icon/apple-touch-icon.png">'));
// Deferred, it still runs before the framework's asynchronous adapter load and any install offer.
assert.ok(head.includes('<script src="/install-app.js" defer></script>'));
assert.equal(result.match(/install-app\.js/g).length,1);
assert.match(result,/#launcher\{display:none!important\}/);
assert.match(result,/#launcher-form>:not\(#error\):not\(#status\)\{display:none!important\}/);
assert.match(result,/#launcher:has\(#error:not\(\[hidden\]\)\)\{display:grid!important\}/);
assert.equal(prepareWebShell(result),result);
for(const id of ['launcher','player-name','advanced-settings','launch-fullscreen','error','game-canvas'])assert.ok(result.includes('id="'+id+'"'));
assert.throws(()=>prepareWebShell('<html>Unexpected</html>'));
assert.match(fs.readFileSync('Dockerfile','utf8'),/prepare-web-shell.cjs \/opt\/shared-shell\/index.html/);
console.log('PASS: launcher hidden before first paint, blocking game CSS and the loading screen\'s Barlow preloaded in head, Safari\'s install icon and the early install offer script, visible startup failures, preserved framework anchors and idempotent pinned-shell adaptation');
