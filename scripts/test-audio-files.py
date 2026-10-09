#!/usr/bin/env python3
"""Check the real browser parsers against every packaged original sound."""
from pathlib import Path
import json
import struct
import subprocess
import tempfile
import wave
import zipfile

root = Path(__file__).resolve().parent.parent
source = (root/'downstream/wasm/web_audio.c').read_text()
body = source[source.index('static unsigned web_u16'):source.index('static WebSound *web_new')]
support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef struct {int format;const void *data_ptr;unsigned data_len,rate;int bits,channels;unsigned samples,block_size;const void *initial_ptr;} AILSOUNDINFO;
'''
checks = r'''
int main(int argc,char **argv){
 for(int a=1;a<argc;a+=4){
  FILE *f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);int n=ftell(f);rewind(f);
  byte *b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);
  AILSOUNDINFO info;int rate=0,channels=0,duration=0;
  if(!memcmp(b,"RIFF",4)){
   assert(WebAudio_WavInfo(b,n,&info));rate=info.rate;channels=info.channels;
   duration=(int)((double)info.samples*1000/info.rate);
   assert(info.data_ptr>=(void *)b && (byte *)info.data_ptr+info.data_len<=b+n);
   for(int limit=0;limit<32;limit++)assert(!WebAudio_WavInfo(b,limit,&info));
   assert(!WebAudio_WavInfo(b,n-1,&info));
  }else assert(WebAudio_Mp3Info(b,n,&rate,&channels,&duration));
  assert(rate==atoi(argv[a+1]));assert(channels==atoi(argv[a+2]));
  assert(abs(duration-atoi(argv[a+3]))<60);
  free(b);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-audio-files-') as folder:
    path = Path(folder)
    (path/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
    args=[str(path/'test')]
    count=0
    with zipfile.ZipFile(root/'data/browser/main/cod2_browser_audio.iwd') as archive:
        for name in archive.namelist():
            if not name.endswith(('.wav','.mp3')): continue
            target=path/f'{count}{Path(name).suffix}'
            target.write_bytes(archive.read(name))
            if name.endswith('.wav'):
                with wave.open(str(target)) as wav:
                    rate,channels=wav.getframerate(),wav.getnchannels()
                    duration=int(wav.getnframes()*1000/rate)
            else:
                result=json.loads(subprocess.check_output(['ffprobe','-v','quiet','-show_streams','-of','json',str(target)]))['streams'][0]
                rate,channels=int(result['sample_rate']),result['channels']
                duration=int(float(result['duration'])*1000)
            args.extend([str(target),str(rate),str(channels),str(duration)])
            count+=1
    subprocess.run(args,check=True)
print(f'PASS: {count} unchanged original WAV/MP3 files, rates/channels/durations and truncated WAV rejection under ASan/UBSan')
