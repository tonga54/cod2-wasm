#!/usr/bin/env python3
"""Bake the owner's modeled shutter surfaces for shared retail brush/model UVs."""
from pathlib import Path
import argparse,hashlib,json,zipfile,struct
import numpy as np
from PIL import Image
from remix_shutter_bake import read_mesh,project,paired
from remix_joinery import orient
from browser_remix_assets import replace_material,normal_image,specular_image,write_iwi,cstring
from browser_remix_shutters import PACK,MATERIALS
from remix_weapon_finish import decode_iwi

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source',type=Path)
parser.add_argument('--size',type=int,choices=(512,1024,2048),default=2048)
args=parser.parse_args();base=args.source/'rtx-remix/mods/gameReadyAssets';sources={}
mesh_name='Level_Specific/Carentan/Carentan_Building_006.usd'
def register(name):
 p=base/name;sources[name]=hashlib.sha256(p.read_bytes()).hexdigest();return p
textures={}
for channel,suffix in [('color','BaseColor.a'),('normal','OTH_Normal.n'),('rough','Roughness.r'),('metal','Metallic.m')]:
 with Image.open(register('Level_Specific/Carentan/textures/Shutters_bare_'+suffix+'.rtex.dds')) as im:
  textures[channel]=np.asarray(im.convert('RGB' if channel in ('color','normal') else 'L'))
print('Projecting owner shutter geometry and PBR maps',flush=True)
images,projection=project(*read_mesh(register(mesh_name),'Shutter_Window_L078'),textures,(args.size//4,args.size))
entries={};originals={};maps={};families={
 'materials/v_shutter_01':('shutter_single_side',False,True,'v_shutter_brown'),
 'materials/v_shutter_02':('shutter_pair_side',True,True,'v_window_shut_02'),
 'materials/mtl_caen_window_shutter_03':('shutter_single',False,False,'caen_window_shutter_03')}
archives=[zipfile.ZipFile(p) for p in sorted((root/'data/main').glob('*.iwd'))]
index={n:(z,n) for z in archives for n in z.namelist()}
for material,(family,pair,sideways,color_name) in families.items():
 z,n=index[material];data=z.read(n);originals[material]=hashlib.sha256(data).hexdigest()
 z,n=index['images/'+color_name+'.iwi']
 if decode_iwi(z.read(n)).getchannel('A').getextrema()!=(255,255):raise ValueError('Original shutter has opacity')
 transformed={key:orient(paired(image,key=='normal') if pair else image,sideways,key=='normal') for key,image in images.items()}
 color=transformed['color'].convert('RGBA');color.putalpha(255)
 spec=np.asarray(specular_image(transformed['rough'])).copy()
 spec[:,:,:3]=np.rint(10+np.asarray(transformed['metal'],dtype=float)[:,:,None]*60/255).astype(np.uint8)
 detail=Image.new('RGBA',color.size,(128,128,128,255));detail.putalpha(transformed['height'])
 for channel,image,compression in [('c',color,'DXT1'),('n',normal_image(transformed['normal']),'DXT5'),
                                    ('s',Image.fromarray(spec),'DXT5'),('d',detail,'DXT5')]:
  entries[f'images/browser_remix/{family}_{channel}.iwi']=write_iwi([image],compression)
 entries[material]=replace_material(data,family,height=True)
with zipfile.ZipFile(root/'data/browser/main/cod2_browser_renderer.iwd') as z:
 for name in z.namelist():
  if not name.endswith('.d3dbsp'):continue
  data=z.read(name);length,offset=struct.unpack_from('<II',data,8)
  names={cstring(data,i) for i in range(offset,offset+length,72)}
  used=sorted(names&{m.removeprefix('materials/') for m in MATERIALS})
  if used:maps[Path(name).stem]=used
report={'schema':1,'sources':sources,'sourceMesh':'Shutter_Window_L078','projection':projection,
 'size':args.size,'maps':maps,'originals':originals,'entries':{n:hashlib.sha256(d).hexdigest() for n,d in entries.items()}}
target=root/'data/main'/PACK;temporary=target.with_suffix('.tmp')
with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED) as z:
 for name,data in sorted(entries.items()):
  info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
 z.writestr('shutters-report.json',json.dumps(report,indent=2)+'\n')
temporary.replace(target);target.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
for z in archives:z.close()
print('Converted shutter surfaces:',json.dumps(maps),flush=True)
