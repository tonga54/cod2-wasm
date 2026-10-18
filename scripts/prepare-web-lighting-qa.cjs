// Generate a browser GPU fixture from the actual linked production shaders.
// Run after build-web.sh; serve the output directory on localhost.
const fs=require('fs');
const path=require('path');
const root=path.resolve(__dirname,'..');
const output=path.resolve(process.argv[2] || path.join(root,'out/web-lighting-qa'));
fs.mkdirSync(output,{recursive:true});
const linked=fs.readFileSync(path.join(root,'out/cod2-wasm-core/site/cod2.js'),'utf8');
const start=linked.indexOf('          // COD2_POINT_LIGHT_SHADER:');
const end=linked.indexOf('          var vsSource =',start);
if(start<0 || end<=start) throw Error('Build the production WebGL client first');
const generate=Function('GLImmediate','GLEmulation','fsTexEnvPass','texUnitVaryingList=""',linked.slice(start,end)+'\nreturn {pointDefs,pointPass,fsTexEnvPass};');
const shaders=[];
for(const model of [false,true])for(const matrix of [false,true]){
 const uv=matrix?'(u_textureMatrix0 * v_texCoord0).xy':'v_texCoord0.xy';
 const s=generate.call({usedTexUnitList:[0]}, {useTextureMatrix:matrix,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:model},`gl_FragColor=texture2D(u_texUnit0, ${uv}) * vec4(.2,.2,.2,1.);`);
 const fragment=`precision mediump float; uniform sampler2D u_texUnit0; uniform mat4 u_textureMatrix0; varying vec4 v_texCoord0; ${s.pointDefs} void main(){${s.fsTexEnvPass}\n${s.pointPass}}`;
 const convert=s=>s.replace(/varying/g,'in').replace(/texture2D/g,'texture').replace(/gl_FragColor/g,'result');
 shaders.push({model,matrix,fragment:'#version 300 es\nout highp vec4 result;\n'+convert(fragment)});
}
fs.writeFileSync(path.join(output,'lighting-qa.html'), `<!doctype html><meta charset="utf-8"><title>Lighting GPU validation</title><style>body{background:#172027;color:#eee;font:18px system-ui}canvas{border:1px solid #ccc;width:256px;height:256px;image-rendering:pixelated}section{display:inline-block;margin:15px}pre{white-space:pre-wrap}</style><h1>Existing lighting: GPU checks</h1><div id="scenes"></div><pre id="result">Running</pre><script>
const shaders=${JSON.stringify(shaders)};
const results=[];
function check(condition,message){if(!condition)throw Error(message);results.push(message)}
try {
 for(const shader of shaders){
  const section=document.createElement('section');section.innerHTML='<p>'+(shader.model?'Model':'World')+' / '+(shader.matrix?'matrix UV':'plain UV')+'</p>';
  const c=document.createElement('canvas');c.width=c.height=32;section.append(c);document.querySelector('#scenes').append(section);
  const gl=c.getContext('webgl2',{antialias:false,preserveDrawingBuffer:true});check(!!gl,'WebGL2 available');
  const compile=(type,source)=>{const s=gl.createShader(type);gl.shaderSource(s,source);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s};
  const vs='#version 300 es\\nprecision highp float;out highp vec3 v_cod2Eye;out mediump vec3 v_cod2Normal;out mediump vec4 v_texCoord0;uniform float normalZ;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);v_texCoord0=vec4(p,0.,1.);v_cod2Eye=vec3(p*2.-1.,0.);v_cod2Normal=vec3(0.,0.,normalZ);}';
  const p=gl.createProgram();gl.attachShader(p,compile(gl.VERTEX_SHADER,vs));gl.attachShader(p,compile(gl.FRAGMENT_SHADER,shader.fragment));gl.linkProgram(p);check(gl.getProgramParameter(p,gl.LINK_STATUS),'Production '+(shader.model?'model':'world')+' shader linked, matrix='+shader.matrix);gl.useProgram(p);
  const tex=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,tex);gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,2,2,0,gl.RGBA,gl.UNSIGNED_BYTE,new Uint8Array(16).fill(255));gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAX_LEVEL,0);
  gl.uniformMatrix4fv(gl.getUniformLocation(p,'u_textureMatrix0'),false,new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]));
  gl.uniform1i(gl.getUniformLocation(p,'u_cod2LightCount'),1);gl.uniform4fv(gl.getUniformLocation(p,'u_cod2LightEye'),new Float32Array([0,0,100,1/40000,0,0,0,0,0,0,0,0,0,0,0,0]));gl.uniform4fv(gl.getUniformLocation(p,'u_cod2LightColor'),new Float32Array([1,.7,.3,0,0,0,0,0,0,0,0,0,0,0,0,0]));
  function draw(z){gl.uniform1f(gl.getUniformLocation(p,'normalZ'),z);gl.drawArrays(gl.TRIANGLES,0,3);const px=new Uint8Array(4);gl.readPixels(16,16,1,1,gl.RGBA,gl.UNSIGNED_BYTE,px);return [...px]}
  const front=draw(1),back=draw(-1);check(front[0]>150 && front[3]===255,'Front face illuminated; partial mip texture stays visible');
  check(shader.model?back[0]>=50&&back[0]<=52:back[0]===front[0],shader.model?'Back side retains ambient instead of glowing':'World illumination retains derivative-normal behavior');
  check(gl.getError()===gl.NO_ERROR,'No WebGL errors');
  if(shader.model)draw(-1);
 }
 document.querySelector('#result').textContent=JSON.stringify({passed:true,checks:results},null,2);
}catch(e){document.querySelector('#result').textContent=JSON.stringify({passed:false,error:String(e),checks:results},null,2)}
</script>`);

console.log(path.join(output,'lighting-qa.html'));
