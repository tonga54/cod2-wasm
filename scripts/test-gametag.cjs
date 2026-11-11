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
// The canvas defaults to 1920x1080; a profile saved with the old 1280x720
// default takes it once, then keeps whatever the player picks.
assert.match(files.get(path),/seta r_mode "1920x1080"/);
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_mode "1280x720"\r\nseta snd_volume "0.5"');
context.prepareProfile(ui,memfs);
assert.ok(!files.get(path).includes('1280x720') && files.get(path).includes('seta snd_volume "0.5"'));
assert.equal(storage.get('cod2-wasm-profile-v1'),'\nseta snd_volume "0.5"');
assert.equal(storage.get('cod2-wasm-display-v2'),'1');
storage.set('cod2-wasm-profile-v1','seta r_mode "1280x720"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_mode "1280x720"\n'));
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_mode "1600x900"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_mode "1600x900"\n'));
// Color, normal and specular maps default to full resolution; a profile saved
// with the old half-resolution default takes it once, then keeps the player's pick.
assert.match(files.get(path),/seta r_picmip "0"\nseta r_picmip_bump "0"\nseta r_picmip_spec "0"\n/);
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_picmip_bump "1"\r\nseta snd_volume "0.5"\nseta r_picmip_spec "2"\nseta r_picmip "0"');
context.prepareProfile(ui,memfs);
assert.ok(!/r_picmip_bump "1"/.test(files.get(path)));
assert.ok(files.get(path).endsWith('\nseta snd_volume "0.5"\nseta r_picmip_spec "2"\nseta r_picmip "0"\n'), 'a chosen level stays');
assert.equal(storage.get('cod2-wasm-material-maps-v2'),'1');
storage.set('cod2-wasm-profile-v1','seta r_picmip_bump "1"\nseta r_picmip_spec "2"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_picmip_bump "1"\nseta r_picmip_spec "2"\n'));
// A profile saved with the old 1.5x or 2x model detail and grass distance takes
// the new 3x default once, then keeps whatever the player sets.
for (const legacy of ['1.5', '2']) {
  storage.clear();
  storage.set('cod2-wasm-profile-v1',`seta r_lodDistanceScale "${legacy}"\r\nseta snd_volume "0.5"`);
  context.prepareProfile(ui,memfs);
  assert.ok(!files.get(path).includes('r_lodDistanceScale') && files.get(path).includes('seta snd_volume "0.5"'));
  assert.equal(storage.get('cod2-wasm-draw-distance-v2'),'1');
  storage.set('cod2-wasm-profile-v1',`seta r_lodDistanceScale "${legacy}"`);
  context.prepareProfile(ui,memfs);
  assert.ok(files.get(path).endsWith(`seta r_lodDistanceScale "${legacy}"\n`));
}
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_lodDistanceScale "4"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_lodDistanceScale "4"\n'), 'a chosen distance stays');
// A profile saved with the old 720-line 3D scene height takes the new 1080-line
// default once, then keeps whatever the player sets.
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_sceneHeight "720"\r\nseta snd_volume "0.5"');
context.prepareProfile(ui,memfs);
assert.ok(!files.get(path).includes('r_sceneHeight') && files.get(path).includes('seta snd_volume "0.5"'));
assert.equal(storage.get('cod2-wasm-scene-height-v1'),'1');
storage.set('cod2-wasm-profile-v1','seta r_sceneHeight "720"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_sceneHeight "720"\n'));
// A profile saved with the old 0.65 sun shading and 1.0 relief defaults takes
// the stronger WWII lighting defaults once, then keeps whatever the player sets.
storage.clear();
storage.set('cod2-wasm-profile-v1','seta r_sunShading "0.65"\r\nseta r_cavity "1"\nseta r_detailRelief "1.0"\nseta snd_volume "0.5"');
context.prepareProfile(ui,memfs);
assert.ok(!files.get(path).includes('r_sunShading') && !files.get(path).includes('r_cavity') && !files.get(path).includes('r_detailRelief') && files.get(path).includes('seta snd_volume "0.5"'));
assert.equal(storage.get('cod2-wasm-lighting-v1'),'1');
storage.set('cod2-wasm-profile-v1','seta r_sunShading "0.65"\nseta r_cavity "1"');
context.prepareProfile(ui,memfs);
assert.ok(files.get(path).endsWith('seta r_sunShading "0.65"\nseta r_cavity "1"\n'));
storage.clear();
context.prepareProfile(ui,memfs);
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
console.log('PASS: native name, key bindings, graphics, mouse and sound persist; legacy name migrates, stored values win, disabled storage is handled; the canvas defaults to 1920x1080 and profiles holding the old 1280x720 default take it once; normal and specular maps default to half resolution, taken once by profiles holding full resolution; profiles holding the old 1.5x draw distance take the new default once');
