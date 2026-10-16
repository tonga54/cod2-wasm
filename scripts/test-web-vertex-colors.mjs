// Compare actual linked vertex shaders against the former CPU byte conversion.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [baseline,candidate,playwrightModule,output='out/vertex-color-qa']=process.argv.slice(2);
if(!playwrightModule)throw Error('Usage: BASELINE_JS CANDIDATE_JS PLAYWRIGHT_MODULE [OUTPUT]');
const variants=[];
for(const lit of [false,true])for(const mask of (lit?[0,1,15,255]:[0])){
 const shaders={lit,mask};
 for(const [name,file]of Object.entries({before:baseline,after:candidate})){
  const s=fs.readFileSync(file,'utf8');
  const start=s.indexOf('          var vsLightingDefs = "";'),end=s.indexOf('          function cod2Glsl300(',start);
  if(start<0||end<0)throw Error('Missing linked vertex generator');
  const generate=Function('GLEmulation','GLImmediate','fsTexEnvPass','texUnitAttribList','texUnitVaryingList','vsPointSizeDefs','vsClipPlaneDefs','vsTexCoordInits','vsFogVaryingInit','vsPointSizeInit','vsClipPlaneInit',s.slice(start,end)+';return vsSource;');
  const vs=generate.call({usedTexUnitList:[]},{lightingEnabled:lit,MAX_LIGHTS:8,lightEnabled:Array.from({length:8},(_,i)=>!!(mask&(1<<i)))},{},'','','','','','','','gl_PointSize = 1.0;','');
  shaders[name]='#version 300 es\n'+vs.replace(/attribute/g,'in').replace(/varying/g,'out mediump');
 }
 variants.push(shaders);
}
const {chromium}=await import(pathToFileURL(path.resolve(playwrightModule)));
const browser=await chromium.launch({headless:false,executablePath:process.env.CHROME_EXECUTABLE||undefined});
try{
 const page=await browser.newPage();await page.setContent('<canvas width="256" height="1"></canvas>');
 const report=await page.evaluate(variants=>{
  const gl=document.querySelector('canvas').getContext('webgl2',{antialias:false,preserveDrawingBuffer:true,premultipliedAlpha:false});
  if(!gl)throw Error('WebGL2 unavailable');let checks=0;
  const assert=(x,m)=>{if(!x)throw Error(m);checks++;};
  function compile(type,src){const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);assert(gl.getShaderParameter(s,gl.COMPILE_STATUS),gl.getShaderInfoLog(s));return s;}
  const fs=compile(gl.FRAGMENT_SHADER,'#version 300 es\nprecision mediump float;in mediump vec4 v_color;out vec4 result;void main(){result=v_color;}');
  const identity=new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]);
  const positions=new Float32Array(256*3),rgba=new Uint8Array(1024);
  for(let i=0;i<256;i++){positions[i*3]=(i+.5)/128-1;rgba.set([(i*73)%256,(i*29+17)%256,(i*13+91)%256,i],i*4);}
  const pos=gl.createBuffer(),color=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,pos);gl.bufferData(gl.ARRAY_BUFFER,positions,gl.STATIC_DRAW);
  const details=[];
  for(const v of variants){
   const programs={};
   for(const name of ['before','after']){
    const p=gl.createProgram();gl.attachShader(p,compile(gl.VERTEX_SHADER,v[name]));gl.attachShader(p,fs);gl.bindAttribLocation(p,0,'a_position');gl.bindAttribLocation(p,1,'a_color');gl.bindAttribLocation(p,2,'a_normal');gl.linkProgram(p);assert(gl.getProgramParameter(p,gl.LINK_STATUS),gl.getProgramInfoLog(p));gl.useProgram(p);
    const u=n=>gl.getUniformLocation(p,n);
    gl.uniformMatrix4fv(u('u_modelView'),false,identity);gl.uniformMatrix4fv(u('u_projection'),false,identity);gl.uniformMatrix3fv(u('u_normalMatrix'),false,new Float32Array([1,0,0,0,1,0,0,0,1]));
    gl.uniform4f(u('u_lightModelAmbient'),.25,.3,.4,1);gl.uniform4f(u('u_materialAmbient'),1,1,1,1);gl.uniform4f(u('u_materialDiffuse'),.7,.6,.5,.8);
    gl.uniform4f(u('u_materialEmission'),.03,.02,.01,0);gl.uniform4f(u('u_materialSpecular'),0,0,0,0);gl.uniform1f(u('u_materialShininess'),1);
    for(let l=0;l<8;l++){gl.uniform4f(u('u_lightAmbient'+l),.01,.02,.03,0);gl.uniform4f(u('u_lightDiffuse'+l),.1,.08,.06,0);gl.uniform4f(u('u_lightSpecular'+l),0,0,0,0);gl.uniform4f(u('u_lightPosition'+l),0,0,1,0);}
    programs[name]=p;
   }
   const draw=(name,order,bytes)=>{
    const p=programs[name];gl.useProgram(p);gl.uniform1i(gl.getUniformLocation(p,'u_cod2ColorOrder'),order);
    gl.bindBuffer(gl.ARRAY_BUFFER,pos);gl.enableVertexAttribArray(0);gl.vertexAttribPointer(0,3,gl.FLOAT,false,12,0);
    gl.bindBuffer(gl.ARRAY_BUFFER,color);gl.bufferData(gl.ARRAY_BUFFER,bytes,gl.STREAM_DRAW);gl.enableVertexAttribArray(1);gl.vertexAttribPointer(1,4,gl.UNSIGNED_BYTE,true,4,0);gl.disableVertexAttribArray(2);gl.vertexAttrib3f(2,0,0,1);
    gl.clear(gl.COLOR_BUFFER_BIT);gl.drawArrays(gl.POINTS,0,256);const out=new Uint8Array(1024);gl.readPixels(0,0,256,1,gl.RGBA,gl.UNSIGNED_BYTE,out);return out;
   };
   const expected=draw('before',0,rgba);
   assert(expected.some(x=>x!==0),'fixture renders nonzero pixels');
   for(const order of [0,1,2,0,2,1]){
    const bytes=new Uint8Array(1024);for(let i=0;i<256;i++){const [r,g,b,a]=rgba.subarray(i*4,i*4+4);bytes.set(order===2?[a,r,g,b]:order===1?[b,g,r,a]:[r,g,b,a],i*4);}
    const actual=draw('after',order,bytes);assert(actual.every((x,i)=>x===expected[i]),`pixel mismatch lit=${v.lit} mask=${v.mask} order=${order}`);
   }
   details.push({lit:v.lit,lightMask:v.mask,colors:256,orders:3});
  }
  assert(gl.getError()===gl.NO_ERROR,'no WebGL errors');return {passed:true,checks,details};
 },variants);
 fs.mkdirSync(output,{recursive:true});fs.writeFileSync(path.join(output,'colors.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{await browser.close();}
