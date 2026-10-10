#!/usr/bin/env python3
"""Exercise real lean input, shared simulation, eye collision and view roll."""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--verify-mutants', action='store_true')
args = parser.parse_args()


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean; typedef unsigned char byte;
typedef float vec_t,vec3_t[3];
#define PMF_MANTLE 4
#define PMF_LADDER 32
#define BUTTON_SPRINT 2
typedef struct {int pm_type,pm_flags,eFlags,clientNum; float leanf,viewHeightCurrent;
    vec3_t origin,viewangles;} playerState_t;
typedef struct {int buttons;byte forwardmove,rightmove;} usercmd_t;
typedef struct {float fraction;int startsolid,allsolid;} trace_t;
typedef struct {int down[2],downtime,msec;byte active,wasPressed,padding[2];} kbutton_t;
static kbutton_t kb[28],sprintButton;
static void IN_KeyDown(kbutton_t *b){b->active=1;}
static void IN_KeyUp(kbutton_t *b){b->active=0;}
typedef struct {int keyCatchers,cgameInShellshock;struct {struct {int pm_type;} ps;} snap;} clientActive_t;
static clientActive_t inputClient,*inputClientPtr=&inputClient;
static void *imp_cl=&inputClientPtr;
static struct {struct {int enabled;} current;} bypassStorage,*cl_bypassMouseInput=&bypassStorage;
static void AngleVectors(const vec_t *a,vec_t *f,vec_t *r,vec_t *u) {
    float yaw=a[1]*.017453292519943295f,roll=a[2]*.017453292519943295f;
    assert(!f&&!u&&a[0]==0);
    r[0]=cosf(roll)*sinf(yaw);r[1]=-cosf(roll)*cosf(yaw);r[2]=-sinf(roll);
}
static int traces,startSolid,allSolid;
static float wall=1000;static vec3_t normal={0,1,0};
static float dot(const float *a,const float *b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void Trace(trace_t *tr,const vec_t *start,const vec_t *mins,const vec_t *maxs,
                  const vec_t *end,int skip,int mask) {
    assert(skip==7&&mask==0x02810011);
    for(int i=0;i<3;i++)assert(mins[i]==-4&&maxs[i]==4);
    traces++;assert(traces<=9);
    float radius=4*(fabsf(normal[0])+fabsf(normal[1])+fabsf(normal[2]));
    float a=dot(start,normal)+radius,b=dot(end,normal)+radius;
    *tr=(trace_t){.fraction=1,.startsolid=startSolid||a>wall,.allsolid=allSolid};
    if(b>wall&&b>a)tr->fraction=fmaxf(0,(wall-a)/(b-a));
}
typedef struct {playerState_t *ps;} viewState_t;
typedef struct {int unused;} WeaponDef;
static WeaponDef weapon,*bg_weaponDefs[]={&weapon};
static int BG_GetViewmodelWeaponIndex(playerState_t *p){return 0;}
static void BG_AddViewDamageKick(viewState_t *v,vec_t *a,WeaponDef *w){a[2]+=1;}
static void BG_AddViewIdle(viewState_t *v,vec_t *a,WeaponDef *w){a[0]+=2;}
static void BG_AddScopedViewBob(viewState_t *v,vec_t *a,WeaponDef *w){a[1]+=3;}
static void BG_AddAdsViewBob(viewState_t *v,vec_t *a,WeaponDef *w){a[2]+=4;}
'''
body = ''.join(function('src/PC/universal/q_shared.c', n) for n in
               ('GetLeanFraction_core', 'GetLeanFraction', 'AddLeanToPosition'))
body += ''.join(function('src/PC/bgame/bg_pmove.c', n) for n in
                ('PM_VectorCopy', 'PM_CanLean', 'PM_UpdateLean'))
body += ''.join(function('src/PC/client_mp/cl_input.c', n) for n in
                ('IN_LeanLeft_Down', 'IN_LeanLeft_Up', 'IN_LeanRight_Down',
                 'IN_LeanRight_Up', 'CL_ConsumeButtonPress', 'CL_CmdButtons'))
body += function('src/PC/bgame/bg_weapons.c', 'BG_CalculateViewAngles')

checks = r'''
static playerState_t Player(int stance,float yaw) {
    return (playerState_t){.clientNum=7,.pm_flags=stance,
        .viewHeightCurrent=stance==1?11:stance==2?40:60,
        .origin={10,20,30},.viewangles={0,yaw,0}};
}
static void Tick(playerState_t *p,int ms,int buttons) {
    usercmd_t cmd={.buttons=buttons};traces=0;
    PM_UpdateLean(p,ms,&cmd,Trace);assert(p->leanf>=-1&&p->leanf<=1);
}
static void Eye(playerState_t *p,vec3_t out) {
    memcpy(out,p->origin,sizeof(vec3_t));out[2]+=p->viewHeightCurrent;
    AddLeanToPosition(out,p->viewangles[1],p->leanf,16,20);
}
int main(void) {
    /* Held keys survive command creation and release without losing the other. */
    usercmd_t cmd={0};IN_LeanLeft_Down();CL_CmdButtons(&cmd);assert(cmd.buttons==0x40);
    cmd=(usercmd_t){0};CL_CmdButtons(&cmd);assert(cmd.buttons==0x40);
    IN_LeanRight_Down();cmd=(usercmd_t){0};CL_CmdButtons(&cmd);assert(cmd.buttons==0xc0);
    IN_LeanLeft_Up();cmd=(usercmd_t){0};CL_CmdButtons(&cmd);assert(cmd.buttons==0x80);
    IN_LeanRight_Up();cmd=(usercmd_t){0};CL_CmdButtons(&cmd);assert(cmd.buttons==0);

    int schedules=0,collisionCases=0;
    for(int stance=0;stance<3;stance++)for(int direction=-1;direction<=1;direction+=2)
    for(int ms=1;ms<=66;ms++) {
        playerState_t client=Player(stance,0),server=client;
        int buttons=direction<0?0x40:0x80;
        for(int t=0;t<200;) {
            int dt=fminf(ms,200-t);Tick(&client,dt,buttons);Tick(&server,dt,buttons);t+=dt;
            assert(client.leanf==server.leanf);
            assert(fabsf(client.leanf-direction*t/200.f)<.00001f);
        }
        assert(fabsf(client.leanf-direction)<.00001f);
        for(int t=0;t<400;) {
            int dt=fminf(ms,400-t);Tick(&client,dt,direction<0?0x80:0x40);t+=dt;
            assert(fabsf(client.leanf-(direction-direction*t/200.f))<.00002f);
        }
        for(int t=0;t<200;t+=ms)Tick(&client,ms,0);
        assert(client.leanf==0);schedules++;
    }
    for(int button=0;button<2;button++) {
        playerState_t p=Player(0,0);p.leanf=1;
        Tick(&p,100,button?0x40080:0xc0);assert(p.leanf==.5f);
        Tick(&p,100,button?0x40080:0xc0);assert(p.leanf==0);
    }
    for(int mode=-1;mode<10;mode++) {
        playerState_t p=Player(0,0);p.pm_type=mode;p.leanf=1;Tick(&p,16,0x80);
        assert(p.leanf==((mode==0||mode==1)?1:0));
    }
    for(int reason=0;reason<6;reason++) {
        playerState_t p=Player(0,0);p.leanf=1;
        if(reason<3)p.pm_flags=(int[]){4,32,0x10000}[reason];
        else if(reason<5)p.eFlags=(int[]){0x100,0x200}[reason-3];
        else {usercmd_t c={.buttons=0x80};PM_UpdateLean(&p,16,&c,0);assert(p.leanf==0);continue;}
        Tick(&p,16,0x80);assert(p.leanf==0);
    }
    /* An eye-sized hull must remain on the safe side of angled walls/floors,
     * even while turning from an already fully leaned pose. */
    for(int yaw=0;yaw<360;yaw+=15)for(int stance=0;stance<3;stance++)
    for(int direction=-1;direction<=1;direction+=2)for(int gap=0;gap<=20;gap++) {
        float a=yaw*.017453292519943295f;
        normal[0]=direction*sinf(a);normal[1]=-direction*cosf(a);normal[2]=0;
        playerState_t p=Player(stance,yaw);p.leanf=direction;
        wall=dot(p.origin,normal)+4*(fabsf(normal[0])+fabsf(normal[1]))+gap;
        for(int t=0;t<10;t++) {
            Tick(&p,16,direction<0?0x40:0x80);
            vec3_t eye;Eye(&p,eye);
            assert(dot(eye,normal)+4*(fabsf(normal[0])+fabsf(normal[1]))<=wall+.0001f);
            if(gap>=20)assert(p.leanf==direction);
        }
        Tick(&p,200,0);assert(p.leanf==0);collisionCases++;
    }
    normal[0]=normal[1]=0;normal[2]=-1;
    for(int gap=0;gap<=6;gap++) {
        playerState_t p=Player(1,90);wall=-(p.origin[2]+p.viewHeightCurrent)+4+gap;
        Tick(&p,200,0x80);vec3_t eye;Eye(&p,eye);assert(-eye[2]+4<=wall+.0001f);
    }
    wall=1000;normal[2]=0;normal[1]=1;
    for(int solid=0;solid<2;solid++) {
        playerState_t p=Player(0,0);p.leanf=1;startSolid=solid==0;allSolid=solid==1;
        Tick(&p,16,0x80);assert(p.leanf==0);
    }
    startSolid=allSolid=0;
    for(int direction=-1;direction<=1;direction++) {
        playerState_t p=Player(0,0);p.leanf=direction;
        viewState_t v={.ps=&p};vec3_t angles;BG_CalculateViewAngles(&v,angles);
        assert(angles[0]==2&&angles[1]==3&&angles[2]==direction*16+5);
        vec3_t eye;Eye(&p,eye);
        assert(fabsf(eye[1]-(20-direction*20*cosf(16*.01745329252f)))<.0001f);
        assert(fabsf(eye[2]-(90-abs(direction)*20*sinf(16*.01745329252f)))<.0001f);
    }
    printf("PASS: Q/E command bits, %d lean/reversal/release schedules, %d wall sweeps, floor clearance, interruptions and camera roll\n",schedules,collisionCases);
}
'''
support = support.replace('#include <stdio.h>', '#include <stdio.h>\n#include <stdlib.h>')
variants = [body]
if args.verify_mutants:
    variants += [
        body.replace('ps->leanf = candidate;', 'ps->leanf = 0.0f;'),
        body.replace('trace.fraction < 1.0f', '0'),
        body.replace('GetLeanFraction(ps->leanf) * 16.0f', '0.0f'),
        body.replace('(cmd->buttons & 0xc0) == 0x40', '(cmd->buttons & 0xc0) == 0x80'),
    ]
with tempfile.TemporaryDirectory(prefix='cod2-lean-') as directory:
    p = Path(directory)
    for index, variant in enumerate(variants):
        assert index == 0 or variant != body
        (p / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(p / 'test.c'), '-lm', '-o', str(p / 'test')], check=True)
        result = subprocess.run([str(p / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
        if index == 0:
            print(result.stdout.strip())
if args.verify_mutants:
    print('PASS: former reset, ignored collision, missing view roll and reversed keys all fail')

# The camera, visibility origin, lighting and authoritative shot origin must
# agree about the lean arc. Also check bypass paths release an existing lean.
for path, pattern in [
    ('src/PC/game_mp/g_client_mp.c', r'AddLeanToPosition\(origin, client->ps.viewangles\[1\], client->ps.leanf, 16.0f, 20.0f\)'),
    ('src/PC/cgame_mp/cg_view_mp.c', r'ps->leanf, 16.0f, 20.0f'),
    ('src/PC/server_mp/sv_snapshot_mp.c', r'frame->ps.leanf, 16.0f, 20.0f'),
    ('src/PC/cgame_mp/cg_weapons.c', r'playerState->leanf, 16.0f, 20.0f'),
]:
    assert re.search(pattern, (root / path).read_text()), path
view = function('src/PC/bgame/bg_pmove.c', 'PM_UpdateViewAngles')
assert view.index('if (!PM_CanLean(ps))') < view.index('if (ps->pm_type == 5)')
assert 'ps->leanf = 0.0f;' in view
