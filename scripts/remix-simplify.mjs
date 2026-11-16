// Offline, attribute-aware reduction of the owner's USD meshes.
import fs from 'node:fs';
import {MeshoptSimplifier} from '../third_party/meshoptimizer/meshopt_simplifier.mjs';
await MeshoptSimplifier.ready;
const input=fs.readFileSync(0);
const vertices=input.readUInt32LE(0), count=input.readUInt32LE(4), target=input.readUInt32LE(8), error=input.readFloatLE(12);
if(input.length!==16+vertices*32+count*4 || count%3 || target%3)throw Error('Invalid simplifier payload');
const data=new Float32Array(input.buffer.slice(input.byteOffset+16,input.byteOffset+16+vertices*32));
const indices=new Uint32Array(input.buffer.slice(input.byteOffset+16+vertices*32,input.byteOffset+input.length));
const attributes=new Float32Array(vertices*5);
for(let i=0;i<vertices;i++)attributes.set(data.subarray(i*8+3,i*8+8),i*5);
// Allow shading-normal discontinuities to simplify, but keep atlas seams
// protected: collapsing across those interpolates unrelated texture islands.
const remap=MeshoptSimplifier.generatePositionRemap(data,8), seam=new Uint8Array(vertices), lock=new Uint8Array(vertices);
for(let i=0;i<vertices;i++){
 const j=remap[i];
 if(Math.abs(data[i*8+6]-data[j*8+6])>1e-5 || Math.abs(data[i*8+7]-data[j*8+7])>1e-5)seam[j]=1;
}
const coarse=process.argv[2]==='--coarse-uv';
const uvWeight=coarse?Number(process.argv[3]):2;
if(!Number.isFinite(uvWeight)||uvWeight<=0||uvWeight>2)throw Error('Invalid UV importance');
// Distant vehicle LODs may merge tiny atlas islands. Keeping every seam of a
// detailed dashboard/wheel mesh otherwise retains tens of thousands of faces
// on a truck that occupies only a few dozen pixels. Near assets remain locked.
if(!coarse)for(let i=0;i<vertices;i++)if(seam[remap[i]])lock[i]=2;
const normalWeight=coarse ? 0.15 : 0.5;
const [result,actualError]=MeshoptSimplifier.simplifyWithAttributes(indices,data,8,attributes,5,[normalWeight,normalWeight,normalWeight,uvWeight,uvWeight],lock,target,error,['Permissive']);
const header=Buffer.alloc(8);header.writeUInt32LE(result.length);header.writeFloatLE(actualError,4);
process.stdout.write(header);process.stdout.write(Buffer.from(result.buffer,result.byteOffset,result.byteLength));
