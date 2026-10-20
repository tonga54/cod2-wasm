#!/usr/bin/env python3
"""Check the levels the browser's Miles driver sends to Web Audio for 2D and 3D sounds."""
from pathlib import Path
import json
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'downstream/wasm/web_audio.c').read_text()
source = source.replace('#include "common_types.h"\n', '').replace('#include <emscripten.h>\n', '')
support = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct {int format;const void *data_ptr;unsigned data_len,rate;int bits,channels;unsigned samples,block_size;const void *initial_ptr;} AILSOUNDINFO;
typedef struct {AILSOUNDINFO Info;} AILMIXINFO;
#define EM_JS(ret, name, params, ...) ret name params;
'''
checks = r'''
static float sentVolume, sentPan;
int web_audio_init(void){return 1;}
int web_audio_new(void){static int id;return ++id;}
void web_audio_reset(int id){} void web_audio_remove(int id){}
void web_audio_pcm(int id,const void *p,int bytes,int rate,int bits,int channels,int floating){}
void web_audio_encoded(int id,const char *name,const void *p,int bytes){}
void web_audio_play(int id){} void web_audio_stop(int id,int done){}
int web_audio_status(int id){return 2;} int web_audio_time(int id){return 0;}
void web_audio_seek(int id,int ms){} void web_audio_rate(int id,float rate){}
void web_audio_levels(int id,float volume,float pan){sentVolume=volume;sentPan=pan;}
void web_audio_loop(int id,int count){} void web_audio_effect(int id,int effect){}
void WebAudio_ClearCache(void){} void web_audio_shutdown(void){}
int FS_ReadFile(const char *name,void **data){*data=NULL;return -1;}
void FS_FreeFile(void *data){}
static short pcm[4800];
static AILSOUNDINFO sound(int channels){
 AILSOUNDINFO info={0};info.format=1;info.data_ptr=info.initial_ptr=pcm;info.data_len=sizeof pcm;info.rate=48000;
 info.bits=16;info.channels=channels;info.samples=sizeof pcm/(2*channels);info.block_size=2*channels;return info;
}
static void sample(const char *label,int channels,float left,float right){
 WebSound *s=AIL_allocate_sample_handle(NULL);AILSOUNDINFO info=sound(channels);
 AIL_set_3D_sample_info(s,&info);AIL_set_sample_volume_levels(s,left,right);
 float l,r,v,p;AIL_sample_volume_levels(s,&l,&r);AIL_sample_volume_pan(s,&v,&p);
 printf("\"%s\":[%.6f,%.6f,%.6f,%.6f,%.6f],",label,sentVolume,sentPan,l,r,v);
}
int main(void){
 printf("{");
 sample("mono",1,0.4f,0.4f);
 sample("stereo",2,0.8f,0.8f);
 sample("mono_side",1,0.4f,0.1f);
 sample("stereo_side",2,0.8f,0.2f);
 sample("mono_clamped",1,0.7f,0.7f);
 WebSound *stream=AIL_allocate_sample_handle(NULL);AILSOUNDINFO info=sound(1);AIL_set_3D_sample_info(stream,&info);
 AIL_set_stream_volume_levels(stream,0.25f,0.25f);printf("\"mono_stream\":[%.6f,%.6f],",sentVolume,sentPan);
 WebSound *spatial=AIL_allocate_3D_sample_handle(NULL);info=sound(1);AIL_set_3D_sample_info(spatial,&info);
 AIL_set_3D_position(spatial,0,0,1);AIL_set_3D_sample_volume(spatial,0.8f);
 printf("\"spatial_ahead\":[%.6f,%.6f],",sentVolume,sentPan);
 AIL_set_3D_position(spatial,0,3,0);printf("\"spatial_side\":[%.6f,%.6f]",sentVolume,sentPan);
 printf("}\n");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-web-audio-levels-') as folder:
    path = Path(folder)
    (path / 'test.c').write_text(support + source + checks)
    subprocess.run(['cc', '-O1', '-g', '-w', '-fsanitize=address,undefined', str(path / 'test.c'), '-o', str(path / 'test'), '-lm'],
                   check=True)
    result = json.loads(subprocess.check_output([str(path / 'test')]))


def near(actual, expected, label):
    assert all(abs(a - b) < 1e-5 for a, b in zip(actual, expected)), f'{label}: {actual} != {expected}'


# The driver sends a mono 2D sound, such as the player's own shot, at half its
# volume on each side; it must reach Web Audio at the gain a 3D sound of the
# same volume ahead gets, as the Mac port's Miles doubles mono levels.
near(result['mono'], [0.8, 0, 0.4, 0.4, 0.4], 'mono 2D at the driver\'s halved levels')
near(result['spatial_ahead'], [0.8, 0], '3D sound ahead')
assert result['mono'][0] == result['spatial_ahead'][0], 'a mono 2D sound plays as loud as a 3D one ahead'
near(result['stereo'], [0.8, 0, 0.8, 0.8, 0.8], 'stereo 2D keeps its levels')
near(result['mono_side'], [0.8, -0.75, 0.4, 0.1, 0.4], 'a mono 2D sound keeps its balance')
near(result['stereo_side'], [0.8, -0.75, 0.8, 0.2, 0.8], 'a stereo 2D sound keeps its balance')
near(result['mono_clamped'], [1, 0, 0.7, 0.7, 0.7], 'doubled levels stop at full scale')
near(result['mono_stream'], [0.5, 0], 'a mono stream is doubled like a sample')
near(result['spatial_side'], [0.8, 1], 'a 3D sound to one side is not doubled')
helper = re.search(r'static void web_levels\(WebSound \*s\)\n\{.*?\n\}', source, re.S).group(0)
assert 'web_audio_levels' in helper and 'AIL_set_3D_sample_volume' not in helper
print('PASS: mono 2D sounds reach Web Audio at the Mac port\'s doubled levels; stereo and 3D sounds keep theirs')
