#!/usr/bin/env python3
"""Fit the owner's detailed Enfield to the retail skeleton and animations."""
from pathlib import Path
import argparse, hashlib, json, struct, zipfile
import numpy as np
from PIL import Image
from pxr import Usd, UsdGeom, Gf
import browser_attachment_models as A
import browser_explosives as E
import browser_smine as S
from browser_remix_assets import write_iwi, normal_image, specular_image, replace_material, metalness_image
from browser_remix_model import PACK, MODEL, SURFACE

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path)
args=parser.parse_args()
base=args.source/'rtx-remix/mods/gameReadyAssets/Uniform/test'
archives=[zipfile.ZipFile(p) for p in sorted((root/'data/main').glob('*.iwd'))]
index={n.lower():(z,n) for z in archives for n in z.namelist()}
def read(name):
    z,n=index[name.lower()];return z.read(n)
originals={n:read(n) for n in (MODEL,SURFACE)}
model=E.read_xmodel(originals[MODEL]);old=A.read_surfaces(originals[SURFACE])
names,parents,local=A.read_skeleton(read('xmodelparts/viewmodel_enfield2'))
entries={};sources={};surfaces=[];materials=[]
# These are animated clip and cartridge surfaces. Retain their bones/UVs.
for i in (0,1,4):surfaces.append(old[i]);materials.append(model['materials'][0][i])
transform=np.array([[0,.423,0],[-.423,0,0],[0,0,.36]],dtype=np.float64)
offset=np.array([5,0,-1.7]);normal_transform=np.linalg.inv(transform).T
for filename,bone in [('Enfield_002_LP_001.usd',0),('Enfield_002_Bolt_LP_001.usd',1)]:
    path=base/filename;sources[str(path.relative_to(args.source))]=hashlib.sha256(path.read_bytes()).hexdigest()
    stage=Usd.Stage.Open(str(path));cache=UsdGeom.XformCache()
    for prim in stage.Traverse():
        if not prim.IsA(UsdGeom.Mesh):continue
        mesh=UsdGeom.Mesh(prim);matrix=cache.GetLocalToWorldTransform(prim)
        # Geometry and normals first reach the USD world, then the retail bind pose.
        points=np.array([matrix.Transform(Gf.Vec3d(*p)) for p in mesh.GetPointsAttr().Get()])@transform.T+offset
        normals=np.array([matrix.TransformDir(Gf.Vec3d(*n)) for n in mesh.GetNormalsAttr().Get()])@normal_transform.T
        if bone == 1:
            # The donor bolt is an independently placed Remix replacement, not
            # authored in the body's coordinate system. Fit it to the animated
            # retail bolt's bind bounds so its rear cap cannot enter the eye.
            q,translation=local[bone]
            target=np.array([S._add(S._qrot(q,v['pos']),translation) for v in old[3]['vertices']])
            lo,hi=points.min(0),points.max(0)
            fit=(target.max(0)-target.min(0))/np.maximum(hi-lo,1e-6)
            points=(points-lo)*fit+target.min(0)
            normals/=fit
        normals/=np.maximum(np.linalg.norm(normals,axis=1,keepdims=True),1e-12)
        uv=np.array(UsdGeom.PrimvarsAPI(prim).GetPrimvar('st').ComputeFlattened());uv[:,1]=1-uv[:,1]
        ids=np.array(mesh.GetFaceVertexIndicesAttr().Get());counts=mesh.GetFaceVertexCountsAttr().Get()
        assignment=np.full(len(counts),2 if bone else 1)
        for subset in UsdGeom.Subset.GetAllGeomSubsets(mesh):assignment[np.array(subset.GetIndicesAttr().Get())]=int(str(subset.GetPath())[-2:])
        groups={};cursor=0
        for f,count in enumerate(counts):
            group=groups.setdefault(int(assignment[f]),{'vertices':[],'indices':[],'cache':{}})
            corners=[]
            for corner in range(cursor,cursor+count):
                key=(int(ids[corner]),tuple(np.round(normals[corner],5)),tuple(np.round(uv[corner],6)))
                if key not in group['cache']:
                    group['cache'][key]=len(group['vertices']);group['vertices'].append((points[ids[corner]],normals[corner],uv[corner]))
                corners.append(group['cache'][key])
            for j in range(1,count-1):group['indices'].append((corners[0],corners[j],corners[j+1]))
            cursor+=count
        for material,group in sorted(groups.items()):
            verts=group['vertices'];tri=np.array(group['indices'],dtype=np.int64)
            p=np.array([v[0] for v in verts]);n=np.array([v[1] for v in verts]);uv=np.array([v[2] for v in verts])
            edges1=p[tri[:,1]]-p[tri[:,0]];edges2=p[tri[:,2]]-p[tri[:,0]]
            duv1=uv[tri[:,1]]-uv[tri[:,0]];duv2=uv[tri[:,2]]-uv[tri[:,0]]
            det=duv1[:,0]*duv2[:,1]-duv1[:,1]*duv2[:,0]
            inv=np.divide(1,det,out=np.zeros_like(det),where=np.abs(det)>1e-12)
            tangents=(edges1*duv2[:,1,None]-edges2*duv1[:,1,None])*inv[:,None]
            bitangents=(edges2*duv1[:,0,None]-edges1*duv2[:,0,None])*inv[:,None]
            t=np.zeros_like(p);b=np.zeros_like(p)
            for k in range(3):np.add.at(t,tri[:,k],tangents);np.add.at(b,tri[:,k],bitangents)
            t-=n*np.sum(n*t,axis=1,keepdims=True)
            tl=np.linalg.norm(t,axis=1);bad=tl<1e-8
            t[bad]=np.cross(n[bad],np.array([0,0,1]));tl=np.linalg.norm(t,axis=1)
            t[tl<1e-8]=[1,0,0];t/=np.maximum(np.linalg.norm(t,axis=1,keepdims=True),1e-12)
            cross=np.cross(n,t);b=cross*np.where(np.sum(cross*b,axis=1,keepdims=True)<0,-1,1)
            # CoD2 winds triangles into the surface, unlike USD.
            reverse=np.sum(np.cross(edges1,edges2)*n[tri].sum(axis=1),axis=1)>0
            tri[reverse]=tri[reverse][:,[0,2,1]]
            if len(p)>32767:raise ValueError('Surface exceeds signed retail vertex count')
            q,translation=local[bone];qinv=(-q[0],-q[1],-q[2],q[3])
            converted=[]
            for pv,nv,uvv,bv,tv in zip(p,n,uv,b,t):
                pv=S._qrot(qinv,S._sub(pv,translation));nv=S._qrot(qinv,nv);bv=S._qrot(qinv,bv);tv=S._qrot(qinv,tv)
                raw=struct.pack('<3f4B2f3f3f3f',*nv,255,255,255,255,*uvv,*bv,*tv,*pv)
                converted.append({'raw':raw,'bone':bone,'weights':0,'pos':pv,'normal':nv,'uv':uvv})
            # The file stores ushort counts, but the engine consumes signed shorts.
            # Keep each draw comfortably below 32768 triangles and remap its vertices.
            for start in range(0,len(tri),24000):
                used,remapped=np.unique(tri[start:start+24000].flatten(),return_inverse=True)
                surfaces.append({'tile':0,'marker':bone,'vertices':[converted[int(i)] for i in used],
                                 'triangles':remapped.tolist()})
                materials.append(f'browser_remix/enfield_{material}')
            print(filename,material,len(p),'vertices',len(tri),'triangles',flush=True)
entries[SURFACE]=A.write_surfaces(surfaces)
# Keep the authored header/LOD name and all bone bounds, replacing only the material list.
d=originals[MODEL];pos=27
for _ in range(4):pos=d.index(0,pos+4)+1
pos+=8
count=struct.unpack_from('<H',d,pos)[0];end=pos+2
for _ in range(count):end=d.index(0,end)+1
entries[MODEL]=d[:pos]+struct.pack('<H',len(materials))+b''.join(m.encode()+b'\0' for m in materials)+d[end:]
for i in (1,2):
    def load(suffix,size):
        path=base/f'textures/Enfield_002_001_Enfield_0{i}_{suffix}.rtex.dds'
        sources[str(path.relative_to(args.source))]=hashlib.sha256(path.read_bytes()).hexdigest()
        im=Image.open(path);im.thumbnail((size,size),Image.Resampling.LANCZOS);return im.copy()
    family=f'enfield_{i}'
    entries[f'images/browser_remix/{family}_c.iwi']=write_iwi([load('BaseColor.a',2048)],'DXT1')
    entries[f'images/browser_remix/{family}_n.iwi']=write_iwi([normal_image(load('OTH_Normal.n',2048))])
    rough=load('Roughness.r',1024);metal_image=load('Metallic.m',1024)
    metal=np.asarray(metal_image.convert('L'),dtype=np.float32)/255
    entries[f'images/browser_remix/{family}_metal.iwi']=write_iwi([metalness_image(metal_image)],'DXT5')
    spec=np.asarray(specular_image(rough)).copy();spec[:,:,:3]=np.rint(28+metal[:,:,None]*115).astype(np.uint8)
    entries[f'images/browser_remix/{family}_s.iwi']=write_iwi([Image.fromarray(spec,'RGBA')])
    material=bytearray(replace_material(read('materials/mtl_weapon_enfield'),family,metallic=True))
    name=f'browser_remix/{family}'.encode()+b'\0';struct.pack_into('<I',material,0,len(material));material.extend(name)
    entries['materials/browser_remix/'+family]=bytes(material)
report={'schema':1,'sources':sources,'originals':{n:hashlib.sha256(d).hexdigest() for n,d in originals.items()},
        'entries':{n:hashlib.sha256(d).hexdigest() for n,d in entries.items()},
        'vertices':sum(len(s['vertices']) for s in surfaces),'triangles':sum(len(s['triangles'])//3 for s in surfaces),
        'retained':'retail skeleton, tags, animations, clip and cartridge; fitted donor body and bolt'}
target=root/'data/main'/PACK;tmp=target.with_suffix('.tmp')
with zipfile.ZipFile(tmp,'w') as z:
    for n,data in sorted(entries.items()):
        info=zipfile.ZipInfo(n,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
    z.writestr('model-report.json',json.dumps(report,indent=2)+'\n')
tmp.replace(target);target.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
for z in archives:z.close()
print('Converted Enfield:',report['vertices'],'vertices,',report['triangles'],'triangles')
