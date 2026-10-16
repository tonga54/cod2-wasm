// Native profile storage must retain edits instead of reapplying startup values.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('site/native-game-adapter.js', 'utf8');
const functions = source.slice(source.indexOf('  function validGametag('), source.indexOf('  function showOriginalStartup('));
const storage = new Map();
const files = new Map();
const messages = [];
let blocked = false;
const context = vm.createContext({TextEncoder, localStorage: {
 getItem(k) {if(blocked)throw Error();return storage.get(k);},
 setItem(k,v) {if(blocked)throw Error();storage.set(k,v);}
}});
vm.runInContext(`let gametag;const gametagStorageKey='cod2-wasm-gametag';${functions}`, context);
const ui={preferences:{values:()=>({playerName:'Player'})},log:msg=>messages.push(msg)};
const memfs={mkdirTree(){},writeFile:(p,s)=>files.set(p,s)};
const path='/profile/main/players/default/config_mp.cfg';
context.prepareProfile(ui,memfs);
assert.match(files.get(path),/seta name "Player"/);
storage.set('cod2-wasm-gametag','Old name');context.prepareProfile(ui,memfs);
assert.match(files.get(path),/seta name "Old name"/);
const changed='unbindall\nbind w "+forward"\nbind v "+melee"\nbind AUX1 "+reload"\nseta cl_gamepadBindingsInitialized "1"\nseta cl_gamepadLookSensitivity "6.5"\nseta name "New name"\nseta r_mode "1600x900"\nseta sensitivity "3.25"\nseta snd_volume "0.4"\nseta password "private"';
context.saveProfile(ui,changed);context.prepareProfile(ui,memfs);
const restored=files.get(path);
assert.ok(restored.indexOf('seta name "New name"')>restored.indexOf('seta name "Old name"'));
for(const line of changed.split('\n').filter(l=>!l.includes('password')))assert.ok(restored.includes(line));
assert.ok(!restored.includes('private'));
context.saveProfile(ui,changed.replace('New name','Another name'));context.prepareProfile(ui,memfs);
assert.ok(files.get(path).includes('Another name'));
blocked=true;context.prepareProfile(ui,memfs);context.saveProfile(ui,changed);
assert.equal(messages.length,1);
assert.ok(!source.includes('await requireGametag'));
for(const name of ['x;quit','x" +connect evil','x\nquit','é'.repeat(16)]) assert.equal(context.validGametag(name),'');
console.log('PASS: native name, key bindings, graphics, mouse and sound persist; legacy name migrates, stored values win, disabled storage is handled');
