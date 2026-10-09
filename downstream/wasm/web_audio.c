#include "common_types.h"
#include <emscripten.h>
#include <stdint.h>
#include <math.h>

extern int FS_ReadFile(const char *, void **);
extern void FS_FreeFile(void *);

typedef struct WebSound {
    struct WebSound *next;
    int id, bits, channels, floating, baseRate, rate, loop, dirty, duration;
    const void *data;
    unsigned int bytes;
    float left, right, volume, position[3];
} WebSound;
static WebSound *webSounds;

EM_JS(int, web_audio_init, (), {
    try {
        Module.cod2Audio = Module.cod2CreateAudio(() => HEAPU8, message => out(message));
        out('[audio] browser output initialized');
        return 1;
    } catch(error) { err('[audio] initialization failed: '+error.message); return 0; }
});
EM_JS(int, web_audio_new, (), { return Module.cod2Audio.create(); });
EM_JS(void, web_audio_reset, (int id), { Module.cod2Audio.reset(id); });
EM_JS(void, web_audio_remove, (int id), { Module.cod2Audio.remove(id); });
EM_JS(void, web_audio_pcm, (int id, const void *p, int bytes, int rate, int bits, int channels, int floating), {
    Module.cod2Audio.pcm(id,p,bytes,rate,bits,channels,floating);
});
EM_JS(void, web_audio_encoded, (int id, const char *name, const void *p, int bytes), {
    Module.cod2Audio.encoded(id,UTF8ToString(name),p,bytes);
});
EM_JS(void, web_audio_play, (int id), { Module.cod2Audio.play(id); });
EM_JS(void, web_audio_stop, (int id, int done), { Module.cod2Audio.stop(id,done); });
EM_JS(int, web_audio_status, (int id), { return Module.cod2Audio.status(id); });
EM_JS(int, web_audio_time, (int id), { return Module.cod2Audio.time(id)|0; });
EM_JS(void, web_audio_seek, (int id, int ms), { Module.cod2Audio.seek(id,ms); });
EM_JS(void, web_audio_rate, (int id, float rate), { Module.cod2Audio.rate(id,rate); });
EM_JS(void, web_audio_levels, (int id, float volume, float pan), { Module.cod2Audio.levels(id,volume,pan); });
EM_JS(void, web_audio_loop, (int id, int count), { Module.cod2Audio.loop(id,count); });
EM_JS(void, WebAudio_ClearCache, (), { if(Module.cod2Audio)Module.cod2Audio.clearCache(); });
EM_JS(void, web_audio_shutdown, (), { if(Module.cod2Audio){Module.cod2Audio.shutdown();Module.cod2Audio=null;} });

static unsigned web_u16(const byte *p) {return p[0] | (unsigned)p[1]<<8;}
static unsigned web_u32(const byte *p) {return web_u16(p) | web_u16(p+2)<<16;}

int WebAudio_WavInfo(const void *data, int size, AILSOUNDINFO *info)
{
    const byte *p=data;
    unsigned end, pos, format=0, channels=0, rate=0, bits=0, length=0;
    const byte *samples=NULL;
    memset(info,0,sizeof(*info));
    if(size<12 || memcmp(p,"RIFF",4) || memcmp(p+8,"WAVE",4))return 0;
    end=web_u32(p+4);
    if(end>(unsigned)size-8 || end<4)return 0;
    end+=8;
    for(pos=12;pos+8<=end;) {
        unsigned n=web_u32(p+pos+4), start=pos+8;
        if(n>end-start)return 0;
        if(!memcmp(p+pos,"fmt ",4)) {
            if(n<16)return 0;
            format=web_u16(p+start);channels=web_u16(p+start+2);
            rate=web_u32(p+start+4);bits=web_u16(p+start+14);
        } else if(!memcmp(p+pos,"data",4)) {samples=p+start;length=n;}
        pos=start+n+(n&1);
    }
    if(!samples || (format!=1 && format!=3) || channels<1 || channels>2 ||
       rate<3000 || rate>192000 || (bits!=8 && bits!=16 && bits!=24 && bits!=32) ||
       (format==3 && bits!=32) || !length || length%(channels*(bits/8)))return 0;
    info->format=format;info->data_ptr=samples;info->initial_ptr=samples;
    info->data_len=length;info->rate=rate;info->bits=bits;info->channels=channels;
    info->samples=length/(channels*(bits/8));info->block_size=channels*(bits/8);
    return 1;
}

/* Stream metadata is needed synchronously by SND_StartAliasStreamOnChannel.
 * The browser decodes the unchanged MP3 payload asynchronously afterward. */
static int WebAudio_Mp3Info(const byte *p, unsigned size, int *rate, int *channels, int *duration)
{
    static const int br1[16]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
    static const int br2[16]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};
    static const int rates[3]={44100,48000,32000};
    unsigned pos=0, frames=0;double seconds=0;
    if(size>=10 && !memcmp(p,"ID3",3)) {
        if((p[6]|p[7]|p[8]|p[9])&0x80)return 0;
        pos=10+((unsigned)p[6]<<21)+((unsigned)p[7]<<14)+((unsigned)p[8]<<7)+p[9];
    }
    while(pos+4<=size) {
        unsigned h=(unsigned)p[pos]<<24|(unsigned)p[pos+1]<<16|(unsigned)p[pos+2]<<8|p[pos+3];
        int version=(h>>19)&3, layer=(h>>17)&3, bi=(h>>12)&15, ri=(h>>10)&3;
        if((h&0xffe00000)!=0xffe00000 || version==1 || layer!=1 || ri==3 || bi==0 || bi==15) {pos++;continue;}
        int hz=rates[ri]/(version==3?1:version==2?2:4);
        int length=(version==3?144000:72000)*(version==3?br1[bi]:br2[bi])/hz+((h>>9)&1);
        if(length<4 || (unsigned)length>size-pos)break;
        if(!frames){*rate=hz;*channels=((h>>6)&3)==3?1:2;}
        seconds+=(version==3?1152.0:576.0)/hz;
        frames++;pos+=length;
    }
    *duration=(int)(seconds*1000.0);
    return frames>0;
}

static WebSound *web_new(void)
{
    WebSound *s=calloc(1,sizeof(*s));
    if(!s)return NULL;
    s->id=web_audio_new();s->bits=16;s->channels=1;s->baseRate=s->rate=44100;
    s->loop=1;s->left=s->right=s->volume=1;s->next=webSounds;webSounds=s;
    return s;
}
static void web_prepare(WebSound *s)
{
    if(s && s->dirty && s->data && s->bytes) {
        web_audio_pcm(s->id,s->data,s->bytes,s->baseRate,s->bits,s->channels,s->floating);
        s->duration=(int)((double)s->bytes*8000/(s->bits*s->channels*s->baseRate));
        s->dirty=0;
    }
}
static void web_levels(WebSound *s)
{
    float maximum=fmaxf(s->left,s->right);
    web_audio_levels(s->id,maximum,maximum>0?(s->right-s->left)/maximum:0);
}

int AIL_startup(unsigned buses){(void)buses;return web_audio_init();}
void AIL_shutdown(void){while(webSounds){WebSound *s=webSounds;webSounds=s->next;free(s);}web_audio_shutdown();}
int AIL_set_preference(unsigned n,int value){(void)n;return value;}
char *AIL_last_error(void){return "Browser audio could not decode or open this sound";}
void *AIL_open_digital_driver(unsigned rate,int bits,int channels,unsigned flags){return (void *)1;}
char *AIL_set_redist_directory(const char *dir){return (char *)dir;}
int AIL_digital_CPU_percent(void *driver){return 0;}
void *AIL_allocate_sample_handle(void *driver){return web_new();}
void AIL_release_sample_handle(WebSound *s){if(!s)return;WebSound **p=&webSounds;while(*p && *p!=s)p=&(*p)->next;if(*p){*p=s->next;web_audio_remove(s->id);free(s);}}
void AIL_init_sample(WebSound *s){if(!s)return;web_audio_reset(s->id);s->data=NULL;s->bytes=0;s->duration=0;s->dirty=1;s->floating=0;s->baseRate=s->rate=0;s->loop=1;}
void AIL_set_sample_adpcm_block_size(WebSound *s,unsigned size){(void)s;(void)size;}
void AIL_set_sample_address(WebSound *s,const void *data,unsigned bytes){if(s){s->data=data;s->bytes=bytes;s->dirty=1;}}
void AIL_set_sample_type(WebSound *s,int format,unsigned flags){if(s){s->bits=(format&8)?32:(format&1)?16:8;s->channels=(format&2)?2:1;s->dirty=1;}}
void AIL_stop_sample(WebSound *s){if(s)web_audio_stop(s->id,0);}
void AIL_end_sample(WebSound *s){if(s)web_audio_stop(s->id,1);}
void AIL_resume_sample(WebSound *s){if(s){web_prepare(s);web_audio_play(s->id);}}
void AIL_set_sample_playback_rate(WebSound *s,int rate){if(s && rate>0){if(!s->baseRate)s->baseRate=rate;s->rate=rate;web_audio_rate(s->id,(float)rate/s->baseRate);}}
void AIL_set_sample_volume_levels(WebSound *s,float l,float r){if(s){s->left=l;s->right=r;s->volume=fmaxf(l,r);web_levels(s);}}
void AIL_set_sample_reverb_levels(WebSound *s,float dry,float wet){(void)s;(void)dry;(void)wet;}
void AIL_set_sample_loop_count(WebSound *s,int count){if(s){s->loop=count;web_audio_loop(s->id,count);}}
unsigned AIL_sample_status(WebSound *s){return s?web_audio_status(s->id):2;}
int AIL_sample_playback_rate(WebSound *s){return s?s->rate:0;}
void AIL_sample_volume_levels(WebSound *s,float *l,float *r){if(l)*l=s?s->left:0;if(r)*r=s?s->right:0;}
void AIL_sample_volume_pan(WebSound *s,float *v,float *p){if(v)*v=s?s->volume:0;if(p)*p=s && s->left+s->right>0?s->right/(s->left+s->right):.5f;}
void AIL_set_digital_master_room_type(void *d,int type){(void)d;(void)type;}
void AIL_set_digital_master_reverb_levels(void *d,float dry,float wet){(void)d;(void)dry;(void)wet;}
int AIL_minimum_sample_buffer_size(void *d,int rate,int format){return 4096;}
int AIL_sample_buffer_ready(WebSound *s){return AIL_sample_status(s)==4?-1:0;}
void AIL_load_sample_buffer(WebSound *s,unsigned n,const void *data,unsigned length){AIL_set_sample_address(s,data,length);AIL_resume_sample(s);}
unsigned AIL_sample_position(WebSound *s){return s?(unsigned)((double)web_audio_time(s->id)*s->baseRate*s->channels*(s->bits/8)/1000):0;}
void AIL_set_sample_ms_position(WebSound *s,int ms){if(s)web_audio_seek(s->id,ms);}
void AIL_sample_ms_position(WebSound *s,int *total,int *current){web_prepare(s);if(total)*total=s?s->duration:0;if(current)*current=s?web_audio_time(s->id):0;}

void *AIL_open_stream(void *driver,const char *name,int flags)
{
    void *data=NULL;int size=FS_ReadFile(name,&data);AILSOUNDINFO info;
    if(size<=0)return NULL;
    WebSound *s=web_new();
    if(!s){FS_FreeFile(data);return NULL;}
    if(WebAudio_WavInfo(data,size,&info)) {
        s->baseRate=s->rate=info.rate;s->bits=info.bits;s->channels=info.channels;
        s->duration=(int)((double)info.samples*1000/info.rate);
    } else if(!WebAudio_Mp3Info(data,size,&s->baseRate,&s->channels,&s->duration)) {
        AIL_release_sample_handle(s);FS_FreeFile(data);return NULL;
    }
    s->rate=s->baseRate;s->bytes=size;
    web_audio_encoded(s->id,name,data,size);FS_FreeFile(data);return s;
}
void AIL_close_stream(WebSound *s){AIL_release_sample_handle(s);}
void AIL_pause_stream(WebSound *s,int pause){if(pause)AIL_stop_sample(s);else AIL_resume_sample(s);}
void AIL_set_stream_volume_levels(WebSound *s,float l,float r){AIL_set_sample_volume_levels(s,l,r);}
void AIL_set_stream_reverb_levels(WebSound *s,float dry,float wet){AIL_set_sample_reverb_levels(s,dry,wet);}
void AIL_stream_volume_pan(WebSound *s,float *v,float *p){AIL_sample_volume_pan(s,v,p);}
void AIL_stream_volume_levels(WebSound *s,float *l,float *r){AIL_sample_volume_levels(s,l,r);}
void AIL_set_stream_playback_rate(WebSound *s,int rate){AIL_set_sample_playback_rate(s,rate);}
int AIL_stream_playback_rate(WebSound *s){return AIL_sample_playback_rate(s);}
void AIL_set_stream_loop_count(WebSound *s,int count){AIL_set_sample_loop_count(s,count);}
int AIL_stream_status(WebSound *s){return AIL_sample_status(s);}
void AIL_stream_info(WebSound *s,int *type,int *rate,int *length,int *memory){if(type)*type=s && s->channels==2?3:1;if(rate)*rate=s?s->rate:0;if(length)*length=s?s->bytes:0;if(memory)*memory=s?s->bytes:0;}
void AIL_set_stream_ms_position(WebSound *s,int ms){AIL_set_sample_ms_position(s,ms);}
void AIL_stream_ms_position(WebSound *s,int *total,int *current){AIL_sample_ms_position(s,total,current);}
int AIL_is_3D_stream(WebSound *s){return 0;}
void AIL_set_file_callbacks(void *a,void *b,void *c,void *d){/* The engine FS_ReadFile path already resolves private IWDs. */}
int AIL_size_processed_digital_audio(unsigned rate,unsigned format,int count,const AILMIXINFO *info){return info?(int)info->Info.data_len:0;}
int AIL_process_digital_audio(void *dest,int size,unsigned rate,unsigned format,int count,const AILMIXINFO *info){if(!info || size<(int)info->Info.data_len)return 0;memcpy(dest,info->Info.data_ptr,info->Info.data_len);return info->Info.data_len;}
int AIL_enumerate_3D_providers(unsigned *next,void **provider,const char **name){if(*next)return 0;*next=1;*provider=(void *)1;*name="Browser positional audio";return 1;}
int AIL_open_3D_provider(void *provider){return 0;}
void AIL_close_3D_provider(void *provider){}
void AIL_3D_provider_attribute(void *provider,const char *name,int *value){*value=32;}
void *AIL_allocate_3D_sample_handle(void *provider){return web_new();}
void AIL_stop_3D_sample(WebSound *s){AIL_stop_sample(s);}
void AIL_resume_3D_sample(WebSound *s){AIL_resume_sample(s);}
void AIL_end_3D_sample(WebSound *s){AIL_end_sample(s);}
int AIL_set_3D_sample_info(WebSound *s,const AILSOUNDINFO *info){if(!s||!info)return 0;AIL_init_sample(s);s->data=info->data_ptr;s->bytes=info->data_len;s->bits=info->bits;s->channels=info->channels;s->floating=info->format==3;s->baseRate=s->rate=info->rate;return 1;}
void AIL_set_3D_sample_volume(WebSound *s,float volume){if(s){s->volume=volume;float horizontal=hypotf(s->position[0],s->position[1]);web_audio_levels(s->id,volume,horizontal>0?s->position[1]/horizontal:0);}}
void AIL_set_3D_sample_offset(WebSound *s,unsigned bytes){if(s && s->baseRate)AIL_set_sample_ms_position(s,(int)((double)bytes*8000/(s->baseRate*s->channels*s->bits)));}
void AIL_set_3D_sample_playback_rate(WebSound *s,int rate){AIL_set_sample_playback_rate(s,rate);}
void AIL_set_3D_sample_loop_count(WebSound *s,unsigned count){AIL_set_sample_loop_count(s,count);}
unsigned AIL_3D_sample_status(WebSound *s){return AIL_sample_status(s);}
float AIL_3D_sample_volume(WebSound *s){return s?s->volume:0;}
unsigned AIL_3D_sample_offset(WebSound *s){return AIL_sample_position(s);}
int AIL_3D_sample_playback_rate(WebSound *s){return AIL_sample_playback_rate(s);}
unsigned AIL_3D_sample_length(WebSound *s){return s?s->bytes:0;}
void AIL_set_3D_room_type(void *p,int type){}
void AIL_set_3D_rolloff_factor(void *p,float f){/* Distance attenuation is applied by SND_Attenuate. */}
void AIL_set_3D_distance_factor(void *p,float f){}
void AIL_set_3D_sample_distances(WebSound *s,float maximum,float minimum){}
void AIL_set_3D_sample_effects_level(WebSound *s,float wet){}
void AIL_set_3D_position(WebSound *s,float x,float y,float z){if(s && s!=(void *)-1){s->position[0]=x;s->position[1]=y;s->position[2]=z;AIL_set_3D_sample_volume(s,s->volume);}}
void AIL_set_3D_stream_position(WebSound *s,float x,float y,float z){AIL_set_3D_position(s,x,y,z);}
void AIL_3D_position(WebSound *s,float *x,float *y,float *z){if(s==(void *)-1)s=NULL;if(x)*x=s?s->position[0]:0;if(y)*y=s?s->position[1]:0;if(z)*z=s?s->position[2]:0;}
int AIL_WAV_info(const void *data,AILSOUNDINFO *info){const byte *p=data;if(memcmp(p,"RIFF",4))return 0;unsigned size=web_u32(p+4);return size<67108864?WebAudio_WavInfo(data,size+8,info):0;}
