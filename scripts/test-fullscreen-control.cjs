// Run the F10 fullscreen control against browser transitions.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('site/native-game-adapter.js', 'utf8');
const control = source.slice(source.indexOf('  function startFullscreenControl('), source.indexOf('  function startUpdateNotifier('));
assert.doesNotMatch(control, /createElement|appendChild|cod2-fullscreen-controls/);
// The native menus have no fullscreen option to call back or report to.
assert.doesNotMatch(source, /cod2ToggleFullscreen|_web_client_fullscreen_state/);
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
  sandbox.startFullscreenControl({});
  function key(type='keydown',options={}){
    const event={key:'F10',isTrusted:true,...options,stopped:false,prevented:false,
      preventDefault(){this.prevented=true;},stopImmediatePropagation(){this.stopped=true;}};
    keyEvents.get(type)(event);return event;
  }
  return {key,document,navigator,requests:()=>requests,exits:()=>exits,
    locks:()=>locks,unlocks:()=>unlocks,fail(value){failure=value;},
    finishKeyboard(){finishKeyboard();},
    enter(){document.fullscreenElement=document.documentElement;docEvents.get('fullscreenchange')();resolveEntry();},
    escape(){document.fullscreenElement=null;docEvents.get('fullscreenchange')();},
    deny(){rejectEntry(Error('Denied'));}};
}
const settle=async()=>{await Promise.resolve();await Promise.resolve();};
(async()=>{
  const b=browser();assert.equal(b.requests(),0,'starting the control requests nothing');
  b.navigator.userActivation.isActive=false;b.key();assert.equal(b.requests(),0);
  b.navigator.userActivation.isActive=true;b.key();b.key();
  assert.equal(b.requests(),1,'a press while a request is pending is ignored');
  b.enter();await settle();
  const exit=b.key();assert.ok(exit.stopped&&exit.prevented);await settle();
  assert.equal(b.exits(),1);assert.equal(b.document.fullscreenElement,null);
  b.key('keydown',{repeat:true});b.key('keyup');assert.equal(b.requests(),1);
  b.key();b.enter();await settle();b.escape();assert.equal(b.requests(),2);
  for(const o of [{isTrusted:false},{ctrlKey:true},{altKey:true},{metaKey:true},{shiftKey:true},{key:'Enter'}])assert.equal(b.key('keydown',o).stopped,false);
  assert.equal(b.requests(),2);
  // A denied or throwing request leaves the next press free to try again.
  b.key();b.deny();await settle();
  b.fail('throw');b.key();await settle();
  b.fail();b.key();b.enter();await settle();
  assert.equal(b.requests(),5);assert.ok(b.document.fullscreenElement);
  b.fail('exit');b.key();await settle();assert.equal(b.exits(),2);assert.ok(b.document.fullscreenElement);
  b.fail();b.key();await settle();assert.equal(b.exits(),3);assert.equal(b.document.fullscreenElement,null);
  b.document.fullscreenEnabled=false;b.key();assert.equal(b.requests(),5);
  const locked=browser({keyboardSupported:true});locked.key();locked.enter();await settle();
  assert.equal(locked.locks(),1);assert.equal(locked.key('keydown',{key:'Escape'}).stopped,false);
  assert.equal(locked.key('keydown',{key:'Escape',repeat:true}).stopped,true);
  assert.ok(locked.document.fullscreenElement);locked.key();await settle();assert.equal(locked.unlocks(),1);
  const denied=browser({keyboardSupported:true,denyKeyboard:true});denied.key();denied.enter();await settle();
  assert.ok(denied.document.fullscreenElement);assert.equal(denied.key('keydown',{key:'Escape',repeat:true}).stopped,false);
  const delayed=browser({keyboardSupported:true,delayedKeyboard:true});delayed.key();delayed.enter();delayed.escape();delayed.finishKeyboard();await settle();assert.equal(delayed.unlocks(),2);
  console.log('PASS: F10 enters and exits fullscreen on a user gesture, one request at a time, recovering from denied requests; captured Escape; no HTML controls or native menu bridge');
})().catch(error=>{console.error(error);process.exitCode=1;});
