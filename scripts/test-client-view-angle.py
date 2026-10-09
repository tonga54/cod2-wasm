#!/usr/bin/env python3
from pathlib import Path
import re, subprocess,tempfile
s=(Path(__file__).resolve().parent.parent/'src/PC/game_mp/g_client_mp.c').read_text();a=s.index('void SetClientViewAngle(gentity_t *ent, const vec_t *angle)\n{');b=s.index('\n}\n',a)+3
support=r'''
#include <assert.h>
#include <math.h>
typedef float vec_t;typedef float vec3_t[3];
typedef struct{struct{int pm_flags,eFlags,delta_angles[3];float proneDirection,proneTorsoPitch;vec3_t viewangles;}ps;struct{struct{int angles[3];}cmd;}sess;}gclient_s;
typedef struct{gclient_s *client;struct{vec3_t currentAngles;}r;}gentity_t;
static float AngleNormalize180(float a){while(a>180)a-=360;while(a<-180)a+=360;return a;}
static float AngleNormalize360(float a){while(a<0)a+=360;while(a>=360)a-=360;return a;}
static float AngleDelta(float a,float b){return AngleNormalize180(a-b);}
'''
checks=r'''
int main(){for(int yaw=-360;yaw<=360;yaw++)for(int stance=0;stance<4;stance++){
 gclient_s c={0};gentity_t e={.client=&c};vec3_t a={0,yaw,0};c.ps.pm_flags=stance;
 c.sess.cmd.angles[1]=12345;SetClientViewAngle(&e,a);
 float delta=fabsf(AngleNormalize180(c.ps.viewangles[1]));
 if(stance&1)assert(delta<=45);else assert(c.ps.viewangles[1]==yaw);
 assert(c.ps.delta_angles[1]==(((int)(c.ps.viewangles[1]*182.04444885253906f)&65535)-12345));
 }return 0;}
'''
with tempfile.TemporaryDirectory(prefix='cod2-viewangle-') as d:
 p=Path(d);(p/'t.c').write_text(support+s[a:b]+checks);subprocess.run(['cc','-fsanitize=address,undefined',str(p/'t.c'),'-o',str(p/'t')],check=True);subprocess.run([str(p/'t')],check=True)
print('PASS: 2884 spawn/teleport view angles; standing/crouched yaw unrestricted and prone cap preserved')
