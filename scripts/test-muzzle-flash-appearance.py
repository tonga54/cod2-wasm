#!/usr/bin/env python3
"""Exercise the scoped light-only muzzle variant and the muzzle draw without modifying shared FX templates; the
weapon's flashes are registered as authored (scripts/browser_muzzle_flashes.py), with no enlarged variant."""
from pathlib import Path
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/cgame_mp/cg_weapons.c').read_text()
assert 'CG_EnhanceMuzzleFlash' not in source
assert source.count('weapInfo->viewFlashEffect = (EffectTemplate *)FX_RegisterEffect(weapDef->szViewFlashEffect);')==1
assert source.count('weapInfo->worldFlashEffect = (EffectTemplate *)FX_RegisterEffect(weapDef->szWorldFlashEffect);')==1
assert source.count('muzzleLightEffects[weaponNum] = CG_MuzzleLightsOnly(weapInfo->viewFlashEffect);')==1
light_match=re.search(r'static EffectTemplate \*CG_MuzzleLightsOnly\([^;]*?\)\n\{',source)
light_end=light_match.end();depth=1
while depth:
    depth+=(source[light_end]=='{')-(source[light_end]=='}');light_end+=1
draw_start=source.index('    if (!cent->bMuzzleFlash)',source.index('void CG_AddPlayerWeapon('))
draw_end=source.index('\n}\n',draw_start)
support=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
enum {PT_PARTICLE,PT_ORIENTEDPARTICLE,PT_TAIL,PT_LIGHT,PT_CLOUD,PT_DECAL};
enum {FXCHAN_COLOR,FXCHAN_COLOR_RAND,FXCHAN_ALPHA,FXCHAN_ALPHA_RAND,FXCHAN_SIZE,FXCHAN_SIZE_RAND,FXCHAN_SIZE2};
typedef struct {float mMin,mMax;} FxRange;
typedef struct {const void *curve;FxRange scaleRange;} FxChannel;
typedef struct {int mType,mNonUniformScale,mParentPrimIndex;FxRange mLife,mSpawnCount;FxChannel mFxChannels[24];float spawnFrustumCullRadius;} PrimitiveTemplate;
typedef struct {const char *mEffectName;int mPrimitiveCount;PrimitiveTemplate *mPrimitives[24];} EffectTemplate;
static void *allocated[25];static int allocations;
static void *Hunk_AllocAlignInternal(int size,int align){assert(align==4&&allocations<25);return allocated[allocations++]=malloc(size);}
typedef float vec_t;
typedef struct {int number;} entityState_t;
typedef struct {int bMuzzleFlash;float lerpOrigin[3];} centity_t;
typedef struct {EffectTemplate *viewFlashEffect,*worldFlashEffect;} weaponInfo_t;
static struct {int cubemapShot;float viewModelOrigin[3];} cgStorage,*cg=&cgStorage;
static EffectTemplate *muzzleLightEffects[128],*played;
static unsigned short flashTag=7;static void *s_barrelTags[]={&flashTag};
static int playedCount,scopeLightsOnly;
static int CG_WwiiScopeMuzzleLightsOnly(void){return scopeLightsOnly;}
static int CG_WeaponDObjHandle(int weapon){return 100+weapon;}
static int FX_GetBoneIndex(int handle,int tag){assert(tag==7);return 3;}
static void FX_PlayEntityEffect(void *fx,const float *origin,int unused,int *bolt){assert(origin);assert(bolt[1]==3);played=fx;playedCount++;}
'''
draw='''
static void DrawMuzzle(centity_t *cent,entityState_t *ent,weaponInfo_t *weapInfo,int weaponNum,int bViewModel,int isLocalClientEntity,int bDrawGun){
'''+source[draw_start:draw_end]+'\n}\n'
checks=r'''
static void ClearHunk(void){for(int i=0;i<allocations;i++)free(allocated[i]);allocations=0;}
'''
checks+=r'''
int main(void){
 PrimitiveTemplate primitives[24]={0},saved[24];EffectTemplate fx={.mPrimitiveCount=24};
 for(int i=0;i<24;i++){primitives[i].mType=i%6;primitives[i].mParentPrimIndex=i-1;primitives[i].mLife=(FxRange){100,100};fx.mPrimitives[i]=&primitives[i];}
 memcpy(saved,primitives,sizeof(saved));
 EffectTemplate *lights=CG_MuzzleLightsOnly(&fx);assert(lights&&lights->mPrimitiveCount==4);
 for(int i=0;i<4;i++){assert(lights->mPrimitives[i]->mType==PT_LIGHT);assert(lights->mPrimitives[i]->mParentPrimIndex==-1);assert(lights->mPrimitives[i]->mLife.mMax==100);}
 assert(!memcmp(saved,primitives,sizeof(saved)));
 muzzleLightEffects[1]=lights;
 weaponInfo_t info={.viewFlashEffect=&fx,.worldFlashEffect=&fx};entityState_t ent={.number=2};
 /* A raised WWII scope draws its rifle with its own projection: its flash
  * keeps only the light, which a world-space sprite would not follow. */
 for(int scope=0;scope<2;scope++)
 for(int view=0;view<2;view++)for(int local=0;local<2;local++)for(int gun=0;gun<2;gun++)for(int cube=0;cube<2;cube++) {
  centity_t cent={.bMuzzleFlash=1};cg->cubemapShot=cube;playedCount=0;scopeLightsOnly=scope;
  DrawMuzzle(&cent,&ent,&info,1,view,local,gun);
  int expected=(!local||view)&&(gun||(view&&!cube));
  assert(playedCount==expected);
  if(expected)assert(played==(view?(gun&&!scope?&fx:lights):&fx));
  assert(cent.bMuzzleFlash==(local&&!view));
 }
 ClearHunk();
 assert(!CG_MuzzleLightsOnly(NULL));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-muzzle-appearance-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(support+source[light_match.start():light_end]+draw+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(p/'test.c'),'-lm','-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: flashes registered as authored, scoped light-only FX and all 32 view/local/model/cubemap/WWII-scope combinations under ASan/UBSan')
