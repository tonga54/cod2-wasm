import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [baseline,candidate,playwrightModule,outputDir='out/web-focus-qa']=process.argv.slice(2);
if(!baseline||!candidate||!playwrightModule)throw Error('Usage: node scripts/test-web-focus.mjs BASELINE_JS CANDIDATE_JS PLAYWRIGHT_MODULE [OUTPUT_DIR]');
const {chromium}=await import(pathToFileURL(path.resolve(playwrightModule)));
fs.mkdirSync(outputDir,{recursive:true});
const fragments={};
for(const [name,file] of Object.entries({before:baseline,after:candidate})){
 const source=fs.readFileSync(file,'utf8'),start=source.indexOf('          // COD2_POINT_LIGHT_SHADER:'),end=source.indexOf('          var vsSource =',start);
 const generate=Function('GLImmediate','GLEmulation','fsTexEnvPass','texUnitVaryingList=""',source.slice(start,end)+';return {pointDefs,pointPass,fsTexEnvPass};');
 fragments[name]=[];
 for(const model of [false,true])for(const matrix of [false,true]){
  const uv=matrix?'(u_textureMatrix0 * v_texCoord0).xy':'v_texCoord0.xy';
  const s=generate.call({usedTexUnitList:[0]},{useTextureMatrix:matrix,TexEnvJIT:{getTexUnitType:()=>0x0DE1}},{lightingEnabled:model},`gl_FragColor=texture2D(u_texUnit0, ${uv});`);
  const fragment=`precision mediump float; uniform sampler2D u_texUnit0;uniform mat4 u_textureMatrix0;varying mediump vec4 v_texCoord0;${s.pointDefs}\nvoid main(){${s.fsTexEnvPass}\n${s.pointPass}}`;
  fragments[name].push({model,matrix,source:'#version 300 es\nout highp vec4 result;\n'+fragment.replace(/varying/g,'in').replace(/texture2D/g,'texture').replace(/gl_FragColor/g,'result')});
 }
}
const browser=await chromium.launch({headless:false,executablePath:process.env.CHROME_EXECUTABLE || undefined,args:['--disable-background-timer-throttling']});
try{
 const page=await browser.newPage({viewport:{width:1280,height:800}});
 await page.setContent('<body style="background:#222;color:white;font:18px sans-serif"><h2>Weapon clarity pixel and GPU checks</h2><canvas width="256" height="256"></canvas><pre></pre>');
 const result=await page.evaluate(async fragments=>{
  const canvas=document.querySelector('canvas'),gl=canvas.getContext('webgl2',{antialias:false,preserveDrawingBuffer:true}),checks=[];
  const check=(value,message)=>{if(!value)throw Error(message);checks.push(message);};
  check(!!gl,'WebGL2 available');
  const compile=(type,src)=>{const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);check(gl.getShaderParameter(s,gl.COMPILE_STATUS),gl.getShaderInfoLog(s)||'shader compiled');return s;};
  const vs=compile(gl.VERTEX_SHADER,'#version 300 es\nprecision highp float;out highp vec3 v_cod2Eye;out mediump vec3 v_cod2Normal;out mediump vec4 v_texCoord0;uniform float depth;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);v_texCoord0=vec4(p,0,1);v_cod2Eye=vec3((p-.5)*16.,-depth);v_cod2Normal=vec3(0,0,1);}');
  const programs={};
  for(const [name,shaders]of Object.entries(fragments))programs[name]=shaders.map(s=>{const p=gl.createProgram();gl.attachShader(p,vs);gl.attachShader(p,compile(gl.FRAGMENT_SHADER,s.source));gl.linkProgram(p);check(gl.getProgramParameter(p,gl.LINK_STATUS),gl.getProgramInfoLog(p)||'program linked');gl.useProgram(p);gl.uniformMatrix4fv(gl.getUniformLocation(p,'u_textureMatrix0'),false,new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]));return p;});
  const tex=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,tex);const size=1024,pixels=new Uint8Array(size*size*4);
  for(let y=0;y<size;y++)for(let x=0;x<size;x++){const i=(y*size+x)*4,v=((x>>3)^(y>>3))&1?230:25;pixels[i]=pixels[i+1]=pixels[i+2]=v;pixels[i+3]=102;}
  gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,size,size,0,gl.RGBA,gl.UNSIGNED_BYTE,pixels);gl.generateMipmap(gl.TEXTURE_2D);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
  function setup(p,blur,depth){gl.useProgram(p);gl.uniform1f(gl.getUniformLocation(p,'u_cod2WeaponBlur'),blur);gl.uniform1f(gl.getUniformLocation(p,'depth'),depth);}
  function draw(p,blur,depth){setup(p,blur,depth);gl.drawArrays(gl.TRIANGLES,0,3);const px=new Uint8Array(canvas.width*canvas.height*4);gl.readPixels(0,0,canvas.width,canvas.height,gl.RGBA,gl.UNSIGNED_BYTE,px);return px;}
  const variance=px=>{let mean=0,square=0,n=0;for(let y=24;y<100;y++)for(let x=24;x<100;x++){const v=px[(y*256+x)*4];mean+=v;square+=v*v;n++;}return square/n-(mean/n)**2;};
  const quality=[];
  for(let i=0;i<4;i++){
   const sharp=draw(programs.before[i],0,6),priorBlur=draw(programs.before[i],1,6);
   for(const depth of [2,6,13,40])for(const ads of [0,.25,.5,1]){
    const actual=draw(programs.after[i],ads,depth);
    check(actual.every((v,j)=>v===sharp[j]),`original texture detail and alpha at depth ${depth}, ADS ${ads}, variant ${i}`);
   }
   check(gl.getUniformLocation(programs.after[i],'u_cod2WeaponBlur')===null,'no per-draw material blur uniform, variant '+i);
   if(i>=2)check(variance(sharp)>variance(priorBlur)*1.5,'regression fixture reproduces the old loss of weapon detail, variant '+i);
   quality.push({variant:i,originalVariance:variance(sharp),priorBlurVariance:variance(priorBlur),restoredVariance:variance(draw(programs.after[i],1,6))});
   // Restoring texture detail must retain the existing flash illumination.
   const lit=[];
   for(const name of ['before','after']){
    const p=programs[name][i];gl.useProgram(p);
    gl.uniform1i(gl.getUniformLocation(p,'u_cod2LightCount'),1);
    gl.uniform4fv(gl.getUniformLocation(p,'u_cod2LightEye[0]'),new Float32Array([0,0,2,.0025]));
    gl.uniform4fv(gl.getUniformLocation(p,'u_cod2LightColor[0]'),new Float32Array([.4,.2,.1,0]));
    lit.push(draw(p,0,6));gl.uniform1i(gl.getUniformLocation(p,'u_cod2LightCount'),0);
   }
   check(lit[0].every((v,j)=>v===lit[1][j]),'flash illumination matches original sharp materials, variant '+i);
   check(lit[1].some((v,j)=>j%4<3&&v>sharp[j]),'flash actually lights the fixture, variant '+i);
   check(lit[1].every((v,j)=>j%4!==3||v===102),'flash preserves alpha, variant '+i);
  }
  draw(programs.after[2],1,6);
  check(gl.getError()===gl.NO_ERROR,'no WebGL errors in all pixel checks');
  const ext=gl.getExtension('EXT_disjoint_timer_query_webgl2'),samples=[];
  canvas.width=1280;canvas.height=720;gl.viewport(0,0,1280,720);
  for(const name of ['before','after']){setup(programs[name][2],1,6);for(let j=0;j<20;j++)gl.drawArrays(gl.TRIANGLES,0,3);gl.finish();}
  for(const name of ['before','after','after','before','before','after']){
   setup(programs[name][2],1,6);gl.finish();
   const q=ext?gl.createQuery():null;
   if(q)gl.beginQuery(ext.TIME_ELAPSED_EXT,q);
   const start=performance.now();for(let j=0;j<40;j++)gl.drawArrays(gl.TRIANGLES,0,3);
   if(q)gl.endQuery(ext.TIME_ELAPSED_EXT);gl.finish();
   const wall=(performance.now()-start)/40;let gpu=null;
   if(q){for(let i=0;i<100&&!gl.getQueryParameter(q,gl.QUERY_RESULT_AVAILABLE);i++)await new Promise(r=>setTimeout(r,10));if(gl.getQueryParameter(q,gl.QUERY_RESULT_AVAILABLE)&&!gl.getParameter(ext.GPU_DISJOINT_EXT))gpu=gl.getQueryParameter(q,gl.QUERY_RESULT)/1e6/40;gl.deleteQuery(q);}
   samples.push({name,wallMsPerDraw:wall,gpuMsPerDraw:gpu});
  }
  const debug=gl.getExtension('WEBGL_debug_renderer_info');
  check(gl.getError()===gl.NO_ERROR,'no WebGL errors after benchmark');
  canvas.width=canvas.height=256;gl.viewport(0,0,256,256);draw(programs.after[2],1,6);
  const result={passed:true,checks,quality,samples,renderer:debug?gl.getParameter(debug.UNMASKED_RENDERER_WEBGL):gl.getParameter(gl.RENDERER)};
  document.querySelector('pre').textContent=JSON.stringify(result,null,2);return result;
 },fragments);
 fs.writeFileSync(path.join(outputDir,'gpu.json'),JSON.stringify(result,null,2));await page.screenshot({path:path.join(outputDir,'gpu.png'),fullPage:true});console.log(JSON.stringify(result,null,2));
}finally{await browser.close();}
