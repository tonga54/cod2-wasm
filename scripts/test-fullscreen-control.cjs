// Run the native-menu fullscreen bridge against browser transitions.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('site/native-game-adapter.js', 'utf8');
const control = source.slice(source.indexOf('  function startFullscreenControl('), source.indexOf('  function startUpdateNotifier('));
assert.doesNotMatch(control, /createElement|appendChild|cod2-fullscreen-controls/);
function browser({keyboardSupported=false, denyKeyboard=false, delayedKeyboard=false}={}) {
  const docEvents=new Map(),keyEvents=new Map();
  let requests=0,exits=0,resolveEntry,rejectEntry,failure,locks=0,unlocks=0,finishKeyboard;
  const document={fullscreenElement:null,fullscreenEnabled:true,documentElement:{},
    addEventListener:(name,fn)=>docEvents.set(name,fn),
    async exitFullscreen(){exits++;if(failure==='exit')throw Error('Denied');document.fullscreenElement=null;docEvents.get('fullscreenchange')();}};
  document.documentElement.requestFullscreen=options=>{
    assert.equal(options.navigationUI,'hide');requests++;
    if(failure==='throw')throw Error('Denied');
    return new Promise((resolve,reject)=>{resolveEntry=resolve;rejectEntry=reject;});
  };
  const window={addEventListener(name,fn,capture){assert.equal(capture,true);keyEvents.set(name,fn);}};
  const navigator={userActivation:{isActive:true}};
  if(keyboardSupported)navigator.keyboard={lock(keys){
    assert.deepEqual(Array.from(keys),['Escape']);locks++;
    if(denyKeyboard)return Promise.reject(Error('Denied'));
    if(delayedKeyboard)return new Promise(resolve=>{finishKeyboard=resolve;});
    return Promise.resolve();},unlock(){unlocks++;}};
  const sandbox=vm.createContext({document,window,navigator});vm.runInContext(control,sandbox);
  const bridge=sandbox.startFullscreenControl({});let writes=0,state,uiReady=true;
  const native={_web_client_fullscreen_state(...values){writes++;state=values;return uiReady?1:0;}};
  bridge.sync(native);
  function key(type='keydown',options={}){
    const event={key:'F10',isTrusted:true,...options,stopped:false,prevented:false,
      preventDefault(){this.prevented=true;},stopImmediatePropagation(){this.stopped=true;}};
    keyEvents.get(type)(event);return event;
  }
  return {bridge,key,document,navigator,requests:()=>requests,exits:()=>exits,
    state:()=>state,writes:()=>writes,locks:()=>locks,unlocks:()=>unlocks,
    ready(value){uiReady=value;},fail(value){failure=value;},
    finishKeyboard(){finishKeyboard();},
    enter(){document.fullscreenElement=document.documentElement;docEvents.get('fullscreenchange')();resolveEntry();},
    escape(){document.fullscreenElement=null;docEvents.get('fullscreenchange')();},
    deny(){rejectEntry(Error('Denied'));}};
}
const settle=async()=>{await Promise.resolve();await Promise.resolve();};
(async()=>{
  const b=browser();assert.deepEqual(b.state(),[1,0,0]);assert.equal(b.requests(),0);
  b.bridge.sync();assert.equal(b.writes(),1,'unchanged native flags are not rewritten every frame');
  b.navigator.userActivation.isActive=false;b.bridge.toggle();assert.equal(b.requests(),0);
  b.navigator.userActivation.isActive=true;b.bridge.toggle();b.bridge.toggle();b.key();
  assert.equal(b.requests(),1);assert.deepEqual(b.state(),[1,1,0]);
  b.enter();await settle();assert.deepEqual(b.state(),[2,0,0]);
  b.bridge.toggle();await settle();assert.equal(b.exits(),1);assert.deepEqual(b.state(),[1,0,0]);
  assert.ok(b.key().stopped);b.key('keydown',{repeat:true});b.key('keyup');assert.equal(b.requests(),2);
  b.enter();await settle();b.escape();assert.deepEqual(b.state(),[1,0,0]);
  for(const o of [{isTrusted:false},{ctrlKey:true},{altKey:true},{metaKey:true},{shiftKey:true},{key:'Enter'}])assert.equal(b.key('keydown',o).stopped,false);
  b.bridge.toggle();b.deny();await settle();assert.deepEqual(b.state(),[1,0,1]);
  b.fail('throw');b.bridge.toggle();await settle();assert.deepEqual(b.state(),[1,0,1]);
  b.fail();b.bridge.toggle();b.enter();await settle();assert.deepEqual(b.state(),[2,0,0]);
  b.fail('exit');b.bridge.toggle();await settle();assert.deepEqual(b.state(),[2,0,1]);
  b.fail();b.bridge.toggle();await settle();assert.deepEqual(b.state(),[1,0,0]);
  b.document.fullscreenEnabled=false;b.bridge.sync();assert.equal(b.state()[0],0);
  const before=b.requests();b.bridge.toggle();b.key();assert.equal(b.requests(),before);
  const unready=browser();unready.ready(false);unready.document.fullscreenEnabled=false;unready.bridge.sync();
  const writes=unready.writes();unready.ready(true);unready.bridge.sync();assert.equal(unready.writes(),writes+1,'retry native state until UI initialization completes');
  const locked=browser({keyboardSupported:true});locked.bridge.toggle();locked.enter();await settle();
  assert.equal(locked.locks(),1);assert.equal(locked.key('keydown',{key:'Escape'}).stopped,false);
  assert.equal(locked.key('keydown',{key:'Escape',repeat:true}).stopped,true);
  assert.ok(locked.document.fullscreenElement);locked.bridge.toggle();await settle();assert.equal(locked.unlocks(),1);
  const denied=browser({keyboardSupported:true,denyKeyboard:true});denied.bridge.toggle();denied.enter();await settle();assert.deepEqual(denied.state(),[2,0,0]);
  const delayed=browser({keyboardSupported:true,delayedKeyboard:true});delayed.bridge.toggle();delayed.enter();delayed.escape();delayed.finishKeyboard();await settle();assert.equal(delayed.unlocks(),2);
  console.log('PASS: native fullscreen menu flags, user gestures, pending/denied transitions, initialization retry, F10 and captured Escape without HTML controls');
})().catch(error=>{console.error(error);process.exitCode=1;});
