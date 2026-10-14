#!/usr/bin/env python3
"""Verify rigid screen-space rotation using the production HUD quad builder."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

# Exercise the real vertex builder, rather than mocking rotation. A flat
# non-square arrow must retain its side lengths and right angles at every yaw.
geometry_source = (root / 'src/PC/cgame_mp/cg_drawtools_mp.c').read_text()
geometry = function(geometry_source, 'CG_DrawRotatedPicPhysical')
physical_support = r'''
#include <assert.h>
#include <math.h>
typedef float vec_t,vec2_t[2];typedef int MaterialHandle;
static vec2_t sign[4]={{-1,-1},{1,-1},{1,1},{-1,1}},quad[4];
static void CL_DrawQuadPic(vec2_t *verts,const float *color,int material){
 for(int i=0;i<4;i++)for(int j=0;j<2;j++)quad[i][j]=verts[i][j];
}
'''
physical_checks = r'''
int main(void){
 for(int width=12;width<=128;width+=13)for(int height=9;height<=64;height+=11)
 for(int angle=-360;angle<=360;angle+=5){
  CG_DrawRotatedPicPhysical(20,30,width,height,angle,0,1);
  float ax=quad[1][0]-quad[0][0],ay=quad[1][1]-quad[0][1];
  float bx=quad[2][0]-quad[1][0],by=quad[2][1]-quad[1][1];
  assert(fabsf(hypotf(ax,ay)-width)<.0001f);
  assert(fabsf(hypotf(bx,by)-height)<.0001f);
  assert(fabsf(ax*bx+ay*by)<.003f);
  for(int i=0;i<2;i++)assert(fabsf((quad[0][i]+quad[2][i])/2-(i?30+height*.5f:20+width*.5f))<.0001f);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-flat-hud-') as directory:
    path=Path(directory)
    for variant in (geometry, geometry.replace(' - sh * sign', ' - sw * sign').replace(' + sw * sign', ' + sh * sign')):
        (path/'geometry.c').write_text(physical_support+variant+physical_checks)
        subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined',str(path/'geometry.c'),'-lm','-o',str(path/'geometry')],check=True)
        result=subprocess.run([str(path/'geometry')],capture_output=True,text=True)
        assert (result.returncode==0)==(variant==geometry),result.stderr
print('PASS: native HUD quads stay flat at every bearing; perspective deformation regression rejected')
