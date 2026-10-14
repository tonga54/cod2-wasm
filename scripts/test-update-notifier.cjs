const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const old = 'a'.repeat(64), deployed = 'b'.repeat(64);
let loaded, current = old, available = false, notify = false, reloads = 0, poll, draw, offline = false;
const notices = [], listeners = {};
const document = {hidden:false, addEventListener:(type,fn)=>{listeners[type]=fn;},removeEventListener(){},
  createElement(){throw Error('Client updates must use the native menu, never HTML popups');}};
const sandbox = {document, AbortSignal, location:{reload:()=>{++reloads;}},
  setInterval:fn=>{poll=fn;return 1;},clearInterval(){},addEventListener(){},
  requestAnimationFrame:fn=>{draw=fn;return 1;},cancelAnimationFrame(){},
  fetch:async url=>{
    assert.equal(url,'/version');
    if(offline)throw Error('offline');
    return {ok:true,json:async()=>({current:{buildId:current},update:{available,revision:'c'.repeat(40)}})};
  }};
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync('site/update-notifier.js','utf8'),sandbox);
(async()=>{
 const notifier=sandbox.createCod2UpdateNotifier({loadedBuild:()=>loaded,canNotify:()=>notify,setMenuNotice:v=>notices.push(v)});
 await poll();notifier.render();assert.equal(notices.at(-1),false);
 loaded=old;await poll();available=true;notify=true;await poll();draw();
 assert.equal(notices.at(-1),false,'a repository push never announces a client update');
 notifier.reload();assert.equal(reloads,0);
 current=deployed;notify=false;await poll();assert.equal(notices.at(-1),false,'no notices while loading or connected');
 notify=true;draw();assert.equal(notices.at(-1),true,'disconnect reveals the native menu item without input');
 const count=notices.length;draw();assert.equal(notices.length,count,'unchanged frames do not rewrite native dvars');
 notify=false;notifier.reload();assert.equal(reloads,0,'stale menu clicks cannot end a match');
 draw();assert.equal(notices.at(-1),false);
 notify=true;draw();assert.equal(notices.at(-1),true);assert.equal(reloads,0,'no automatic reload');
 notifier.reload();assert.equal(reloads,1);
 offline=true;await poll();assert.equal(notices.at(-1),true,'offline retains last known update');
 offline=false;current='invalid';await poll();assert.equal(notices.at(-1),false);
 current=old;await poll();assert.equal(notices.at(-1),false);
 console.log('PASS: native menu notices, no repository/popup message, loaded client identity, disconnect transitions, explicit reload and active-match/offline guards');
})().catch(error=>{console.error(error);process.exitCode=1;});
