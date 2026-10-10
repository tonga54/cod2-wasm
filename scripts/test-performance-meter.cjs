const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname,'../site/native-game-adapter.js'),'utf8');
const start = source.indexOf('  function performanceMeter(');
const end = source.indexOf('  function traceWebGl(',start);
assert(start>=0&&end>start);
let now=0,state=1,search='';
let created=0,deleted=0,active=0,disjoint=false;
const output={style:{},dataset:{}};
const document={hidden:false,createElement:()=>output,body:{appendChild(){}}};
const performance={now:()=>now,memory:{usedJSHeapSize:123456}};
const native={_web_client_state:()=>state,HEAPU8:{byteLength:536870912}};
const location={get search(){return search;}};
const make = Function('document','performance','native','location',source.slice(start,end)+'return performanceMeter;')(
  document,performance,native,location);
const context={elements:{canvas:{width:1280,height:720}}};
assert.equal(make(context),undefined);
search='?perfDebug=1';const meter=make(context);
const gl={QUERY_RESULT_AVAILABLE:1,QUERY_RESULT:2,
  getExtension:()=>({TIME_ELAPSED_EXT:3,GPU_DISJOINT_EXT:4}),
  createQuery:()=>++created,deleteQuery(){deleted++;},
  beginQuery(type,query){assert.equal(type,3);assert(!active);active=query;},
  endQuery(type){assert.equal(type,3);assert(active);active=0;},
  getParameter:()=>disjoint,getQueryParameter:(query,p)=>p===1?true:2000000};
meter.attachGL(gl);
function frame(){now+=11.6666667;meter.begin();now+=2;meter.mark("entities");now+=3;meter.mark("render");meter.add("backendDraw",1);meter.add("backendDraw",2);meter.end();}
for(let i=0;i<660;i++)frame();
let report=JSON.parse(output.dataset.report);
assert.equal(report.samples,600);assert(Math.abs(report.fps-60)<.01);
assert(Math.abs(report.mainThreadMs-5)<.01);assert(Math.abs(report.phasesMs.entities-2)<.01);assert(Math.abs(report.phasesMs.render-3)<.01);assert.equal(report.gpuMs,2);
assert.equal(report.wasmMemoryBytes,536870912);assert.equal(report.jsHeapBytes,123456);
assert.deepEqual(report.canvas,[1280,720]);assert.equal(report.visible,true);
assert.equal(report.phasesMs.backendDraw,3); // Draws accumulate per frame and reset before the next one.
assert(created>0&&deleted>0&&created-deleted<4);
assert.equal(report.framesOver33ms,0);
assert(Math.abs(report.mainThreadMsP95-5)<.01);
// A 120 Hz browser with an 85 FPS desktop cap renders every other callback.
// Cheap callbacks must not inflate FPS or dilute CPU/GPU timings.
state=3;
for(let i=0;i<700;i++){
  now+=i%2 ? 8.3333333 : 3.3333333;
  meter.begin();
  if(i%2===0){now+=5;meter.add('backendDraw',5);}
  meter.end();
}
report=JSON.parse(output.dataset.report);
assert(Math.abs(report.fps-60)<.01);
assert(Math.abs(report.mainThreadMs-5)<.01);
assert(Math.abs(report.frameMsP95-1000/60)<.01);
state=2;for(let i=0;i<70;i++)frame();
report=JSON.parse(output.dataset.report);assert.equal(report.state,2);assert(report.samples<70);
disjoint=true;for(let i=0;i<70;i++)frame();
report=JSON.parse(output.dataset.report);assert.equal(report.gpuMs,null);
assert.equal(report.gpuSamples,0);
console.log('PASS: actual rendered FPS including skipped RAF callbacks, CPU percentiles, bounded history and GPU query cleanup/state changes/disjoint rejection');
