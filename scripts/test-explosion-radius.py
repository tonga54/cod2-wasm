#!/usr/bin/env python3
"""Exercise the real splash-damage fallback with a null physics world handle."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()
match = re.search(r'^qboolean G_RadiusDamage\([^;]+?\)\n\{', source, re.M)
assert match
end, depth = match.end(), 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
radius_damage = source[match.start():end]
types = (root / 'src/headers/common_types.h').read_text()
trace_type = re.search(r'struct trace_t \{.*?\n\};', types, re.S).group()

support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef int qboolean;
typedef unsigned char byte;
typedef float vec_t;
typedef float vec3_t[3];
typedef struct trace_t trace_t;
'''+trace_type+r'''
typedef struct {int takedamage;void *client;struct {int bmodel;vec3_t currentOrigin,absmin,absmax;}r;}gentity_t;
static gentity_t g_entities[3];
static struct {int bPlayerIgnoreRadiusDamage;}level;
static int g_phys_world; /* No physics world pointer: this used to crash. */
static int trace_calls,damage_calls,received_damage;
static float visibility,trace_fraction;
static int CM_AreaEntities(const vec_t *mins,const vec_t *maxs,int *list,int size,int type){
 assert(size==1024&&type==-1);list[0]=0;return 1;
}
static float CanDamage(gentity_t *target,const vec_t *origin){return visibility;}
static int LogAccuracyHit(gentity_t *target,gentity_t *attacker){return target->client!=NULL;}
static void G_TraceCapsule(void *out,const vec_t *start,const vec_t *mins,const vec_t *maxs,
                          const vec_t *end,int skip,int mask){
 assert(mins&&maxs);for(int i=0;i<3;i++){assert(mins[i]==0);assert(maxs[i]==0);}
 assert(skip==1023&&mask==0x811);
 trace_t *trace=out;memset(trace,0,sizeof(*trace));trace->fraction=trace_fraction;
 trace->startsolid=1;trace_calls++;
}
static void G_Damage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,
                     const vec_t *dir,const vec_t *origin,int points,int flags,int mod,int loc,int time){
 assert(target==&g_entities[0]&&dir[2]==24&&flags==1);damage_calls++;received_damage=points;
}
'''
scenarios = r'''
int main(void){
 vec3_t origin={0,0,0};gentity_t *target=&g_entities[0],*attacker=&g_entities[1];
 target->takedamage=1;target->client=target;
 target->r.currentOrigin[0]=10;target->r.absmin[0]=5;target->r.absmax[0]=15;
 trace_fraction=.5f;
 /* The close, fully occluded fallback must trace with real zero extents. */
 assert(G_RadiusDamage(origin,attacker,attacker,100,0,100,NULL,4)==1);
 assert(trace_calls==1&&damage_calls==1&&received_damage==9);
 /* At a greater distance, a blocked path does not damage through a wall. */
 target->r.currentOrigin[0]=40;target->r.absmin[0]=35;target->r.absmax[0]=45;
 G_RadiusDamage(origin,attacker,attacker,100,0,100,NULL,4);
 assert(trace_calls==2&&damage_calls==1);
 target->r.currentOrigin[0]=10;target->r.absmin[0]=5;target->r.absmax[0]=15;
 trace_fraction=1;
 G_RadiusDamage(origin,attacker,attacker,100,0,100,NULL,4);
 assert(trace_calls==3&&damage_calls==1);
 /* Ordinary visible splash keeps its full distance attenuation. */
 visibility=1;
 G_RadiusDamage(origin,attacker,attacker,100,0,100,NULL,4);
 assert(trace_calls==3&&damage_calls==2&&received_damage==90);
 G_RadiusDamage(origin,attacker,attacker,100,0,100,target,4);
 assert(damage_calls==2);
 level.bPlayerIgnoreRadiusDamage=1;
 G_RadiusDamage(origin,attacker,attacker,100,0,100,NULL,4);
 assert(damage_calls==2);
 assert(G_RadiusDamage(origin,attacker,NULL,100,0,100,NULL,4)==0);
 puts("Splash damage: null-world fallback, complete trace buffer, occlusion and attenuation passed");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-radius-') as directory:
    temp = Path(directory)
    for name, variant in [
        ('production', radius_damage),
        ('null_extents', radius_damage.replace(
            'G_TraceCapsule(&trace, origin, zero, zero, dest, 0x3ff, 0x811);',
            'G_TraceCapsule(&trace, origin, (const vec_t *)(long)g_phys_world, (const vec_t *)(long)g_phys_world, dest, 0x3ff, 0x811);')),
        ('short_trace', radius_damage.replace('trace_t trace;', 'struct { float fraction; } trace;')),
    ]:
        assert name == 'production' or variant != radius_damage, name
        program, binary = temp / f'{name}.c', temp / name
        program.write_text(support + variant + scenarios)
        subprocess.run(['cc', '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                        str(program), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], text=True, capture_output=True)
        if name == 'production':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0, f'mutant survived: {name}'
            print(f'Splash regression rejected: {name}')
