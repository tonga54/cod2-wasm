#!/usr/bin/env python3
"""Project donor window joinery onto existing open, closed and broken windows."""
from pathlib import Path
import argparse, hashlib, json, struct, zipfile
import numpy as np
from PIL import Image
from remix_shutter_bake import read_mesh, project
from remix_joinery import orient
from remix_window_finish import fit_frame, LAYOUTS
from remix_weapon_finish import decode_iwi
from browser_remix_assets import material_textures, replace_material, normal_image, write_iwi, cstring
from browser_remix_windows import PACK, MATERIALS

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path)
parser.add_argument('--size',type=int,choices=(512,1024,2048),default=2048)
args = parser.parse_args()
base = args.source/'rtx-remix/mods/gameReadyAssets'
sources = {}
def register(name):
    path = base/name
    sources[name] = hashlib.sha256(path.read_bytes()).hexdigest()
    return path
textures = {}
for key,suffix in [('color','BaseColor.a'),('normal','OTH_Normal.n'),('rough','Roughness.r')]:
    with Image.open(register('Level_Specific/Carentan/textures/Window_001_LP_window_001_'+suffix+'.rtex.dds')) as im:
        textures[key] = np.asarray(im.convert('L' if key=='rough' else 'RGB'))
textures['metal'] = np.zeros(textures['rough'].shape,np.uint8)
points,corners,normals,uv = read_mesh(register('Level_Specific/Carentan/Carentan_Building_007.usd'),'window_001_LP102')
points = points[:,[0,2,1]].copy(); normals = normals[:,[0,2,1]].copy()
baked,projection = project(points,corners,normals,uv,textures,(args.size//2,args.size),minimum_coverage=.2)
archives = [zipfile.ZipFile(p) for p in sorted((root/'data/main').glob('*.iwd'))]
index = {n:z for z in archives for n in z.namelist()}
entries,originals = {},{}
for name in LAYOUTS:
    material = 'materials/'+name
    data = index[material].read(material); originals[material] = hashlib.sha256(data).hexdigest()
    maps = {key:image for _,key,image in material_textures(data)}
    images = [decode_iwi(index['images/'+maps[key]+'.iwi'].read('images/'+maps[key]+'.iwi')).transpose(Image.Transpose.ROTATE_90)
              for key in ['colorMap','normalMap','specularMap']]
    packed = np.asarray(images[1])
    images[1] = Image.fromarray(np.dstack((packed[:,:,3],packed[:,:,1],np.full(packed.shape[:2],255,np.uint8))))
    # Rotating the original normal image also rotates its tangent axes.
    n = np.asarray(images[1]).copy(); x,y=n[:,:,0].copy(),n[:,:,1].copy()
    n[:,:,0]=y; n[:,:,1]=255-x; images[1]=Image.fromarray(n)
    result = fit_frame(baked,projection,name,*images,(args.size//2,args.size))
    family = 'window_frame_'+name
    for key,channel in [('color','c'),('normal','n'),('specular','s'),('detail','d')]:
        image = orient(result[key],True,key=='normal')
        if key=='normal': image=normal_image(image)
        # Explicit 4-bit alpha keeps the 0.5 cutout boundary at its authored
        # position; interpolated DXT5 alpha moved some opening edges.
        compression = 'DXT3' if key=='color' and name=='v_window02' else 'DXT5'
        entries['images/browser_remix/'+family+'_'+channel+'.iwi'] = write_iwi([image],compression)
    entries[material] = replace_material(data,family,height=True)
maps = {}
with zipfile.ZipFile(root/'data/browser/main/cod2_browser_renderer.iwd') as archive:
    for name in archive.namelist():
        if not name.endswith('.d3dbsp'): continue
        data=archive.read(name);length,offset=struct.unpack_from('<II',data,8)
        used=sorted({cstring(data,i) for i in range(offset,offset+length,72)} & set(LAYOUTS))
        if used: maps[Path(name).stem]=used
report={'schema':1,'sources':sources,'sourceMesh':'window_001_LP102','projection':projection,
        'size':args.size,'maps':maps,'layouts':LAYOUTS,'originals':originals,
        'entries':{n:hashlib.sha256(d).hexdigest() for n,d in entries.items()}}
target=root/'data/main'/PACK;temporary=target.with_suffix('.tmp')
with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED) as archive:
    for name,data in sorted(entries.items()):
        info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
        archive.writestr(info,data)
    archive.writestr('windows-report.json',json.dumps(report,indent=2)+'\n')
temporary.replace(target);target.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
for archive in archives: archive.close()
print('Projected window surfaces:',json.dumps(maps),flush=True)
