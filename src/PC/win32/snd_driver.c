#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
extern snd_local_t g_snd;

extern float floorf(float x);

extern void FS_FCloseFile(fileHandle_t f);
extern int FS_FOpenFileReadStream(const char *filename, long unsigned int *handle, int uniqueFILE);
extern int FS_Seek(int f, long int offset, int origin);
extern int FS_FTell(int f);
extern int FS_Read(void *buffer, int len, int f);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern const char *FS_ShortOSFilePath(const char *name);
extern int AIL_digital_CPU_percent(void *dig);
extern int AIL_sample_playback_rate(void *S);
extern void AIL_set_sample_playback_rate(void *S, int rate);
extern int AIL_3D_sample_playback_rate(void *S);
extern void AIL_set_3D_sample_playback_rate(void *S, int rate);
extern int AIL_stream_playback_rate(void *stream);
extern void AIL_set_stream_playback_rate(void *stream, int rate);
extern float AIL_3D_sample_volume(void *S);
extern int AIL_3D_sample_length(void *S);
extern void AIL_end_sample(void *S);
extern void AIL_stop_sample(void *S);
extern void AIL_resume_sample(void *S);
extern long unsigned int AIL_sample_status(void *S);
extern void AIL_end_3D_sample(void *S);
extern void AIL_stop_3D_sample(void *S);
extern void AIL_resume_3D_sample(void *S);
extern long unsigned int AIL_3D_sample_status(void *S);
extern void AIL_close_stream(void *stream);
extern void AIL_pause_stream(void *stream, long int onoff);
extern long int AIL_stream_status(void *stream);
extern void AIL_sample_volume_levels(void *S, float *left_level, float *right_level);
extern void AIL_stream_volume_levels(void *S, float *left_level, float *right_level);
extern void AIL_sample_ms_position(void *S, long int *total_milliseconds, long int *current_milliseconds);
extern void AIL_stream_ms_position(void *S, long int *total_milliseconds, long int *current_milliseconds);
extern void AIL_close_3D_provider(void *lib);
extern void AIL_shutdown(void);
extern void AIL_set_stream_reverb_levels(void *S, float dry_level, float wet_level);
extern void AIL_sample_volume_pan(void *S, float *volume, float *pan);
extern void AIL_stream_volume_pan(void *S, float *volume, float *pan);
extern long unsigned int AIL_3D_sample_offset(void *S);
extern void AIL_3D_position(void *obj, float *X, float *Y, float *Z);
extern Bool SND_IsAliasChannel3D(int channel);
extern void AIL_release_sample_handle(void *S);
extern void *AIL_allocate_sample_handle(void *dig);
extern void *AIL_allocate_3D_sample_handle(void *provider);
extern void AIL_init_sample(void *S);
extern void AIL_set_sample_type(void *S, int format, int flags);
extern void AIL_set_sample_volume_levels(void *S, float left, float right);
extern void AIL_set_stream_volume_levels(void *S, float left, float right);
extern int AIL_minimum_sample_buffer_size(void *dig, int rate, int format);
extern void AIL_set_sample_address(void *S, void *ptr, int len);
extern void AIL_set_sample_adpcm_block_size(void *S, int blocksize);
extern void AIL_set_sample_loop_count(void *S, int count);
extern void AIL_set_sample_reverb_levels(void *S, float dry_level, float wet_level);
extern void AIL_set_sample_ms_position(void *S, int ms);
extern long unsigned int AIL_sample_position(void *S);
extern void AIL_set_3D_sample_volume(void *S, float volume);
extern void AIL_set_3D_sample_distances(void *S, float min_dist, float max_dist);
extern void AIL_set_3D_sample_loop_count(void *S, int count);
extern void AIL_set_3D_sample_effects_level(void *S, float level);
extern long int AIL_set_3D_sample_info(void *S, void *info);
extern void AIL_set_3D_sample_offset(void *S, int offset);
extern void AIL_set_3D_position(void *S, float x, float y, float z);
extern void AIL_set_3D_stream_position(void *S, float x, float y, float z);
extern int AIL_is_3D_stream(void *stream);
extern void AIL_set_stream_loop_count(void *S, int count);
extern void AIL_set_stream_ms_position(void *S, int ms);
extern void *AIL_open_stream(void *dig, const char *filename, int flags);
extern void AIL_stream_info(void *stream, int *a, int *b, int *c, int *d);
extern int AIL_sample_buffer_ready(void *S);
extern void AIL_load_sample_buffer(void *S, int buf_num, void *buffer, int size);
extern int AIL_WAV_info(void *data, void *info);
extern int AIL_size_processed_digital_audio(int rate, int format, int num, void *info);
extern long int AIL_process_digital_audio(void *dest, long int size, unsigned long rate, unsigned long format, long int num, void *info);
extern void AIL_set_digital_master_room_type(void *dig, int room_type);
extern void AIL_set_digital_master_reverb_levels(void *dig, float dry, float wet);
extern void AIL_set_3D_room_type(void *provider, int room_type);
extern int AIL_startup(int flags);
extern char * AIL_set_redist_directory(const char *dir);
extern void *AIL_open_digital_driver(int freq, int bits, int channels, int flags);
extern int AIL_set_preference(int pref, int value);
extern int AIL_enumerate_3D_providers(void *provider, void **handle, const char **name);
extern int AIL_open_3D_provider(void *handle);
extern void AIL_3D_provider_attribute(void *provider, const char *name, void *val);
extern void AIL_set_3D_distance_factor(void *provider, float factor);
extern void AIL_set_3D_rolloff_factor(void *provider, float factor);
extern void AIL_set_file_callbacks(void *open_cb, void *close_cb, void *seek_cb, void *read_cb);
extern const char *AIL_last_error(void);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern void *Hunk_AllocNoZeroInternal(int size);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void *Com_GetSoundFileMem(const void *alias);
extern const char *Com_GetSoundFileName(const void *alias);
extern void SND_Update(void);
extern Bool SND_UpdateBackgroundVolume(int streamIdx, int frametime);
extern float SND_Attenuate(void *curve, float dist, float min, float max);
extern float SND_GetLerpedSlavePercentage(float slave);
extern int SND_GetListenerIndexNearestToOrigin(const float *org);
extern void SND_GetCurrent3DPosition(int entnum, float *prevOrg, float *org);
extern float Vec3Normalize(float *v);
extern float Vec3Distance(const float *a, const float *b);
extern int SND_FindFree2DChannel(int entnum, int channel);
extern int SND_FindFree3DChannel(int entnum, int channel);
extern void SND_SetChannelInfo(int index, int entnum, const void *pAlias0, const void *pAlias1, float lerp, const float *org, float volume, float pitch, int channels, int rate, int total_msec, int start_msec, int startDelay, int master, int system);
extern const dvar_t *Dvar_RegisterBool(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterString(const char *name, const char *value, int flags);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern int Dvar_GetInt(const char *name);
extern int stricmp(const char *a, const char *b);

extern const dvar_t *mss_3d_provider;
extern const dvar_t *mss_q3fs;
static struct MssLocal milesGlob;

static HSAMPLE handle_2D_004a3ad4[8];

/* SND channel ids are global: 0..31 spatial, 32..44 streams, 45..52 2D. */
static HSAMPLE SND_2DHandle(int channel)
{
    return channel >= 45 && channel < 53 ? handle_2D_004a3ad4[channel - 45] : NULL;
}

static long unsigned int MSS_FileOpenCallback(const char *pszFilename, long unsigned int *phFileHandle);
static void MSS_FileCloseCallback(long unsigned int hFileHandle);
static long int MSS_FileSeekCallback(long unsigned int hFileHandle, long int offset, long unsigned int type);
static long unsigned int MSS_FileReadCallback(long unsigned int hFileHandle, void *pBuffer, long unsigned int bytes);
void SND_ShutdownDriver(void);
int SND_GetDriverCPUPercentage(void);
void SND_Stop2DChannel(int index);
void SND_Pause2DChannel(int index);
void SND_Unpause2DChannel(int index, int timeshift);
Bool SND_Is2DChannelFree(int index);
void SND_Stop3DChannel(int index);
void SND_Pause3DChannel(int index);
void SND_Unpause3DChannel(int index, int timeshift);
Bool SND_Is3DChannelFree(int index);
void SND_StopStreamChannel(int index);
void SND_PauseStreamChannel(int index);
void SND_UnpauseStreamChannel(int index, int timeshift);
Bool SND_IsStreamChannelFree(int index);
float SND_Get2DChannelVolume(int index);
float SND_Get3DChannelVolume(int index);
float SND_GetStreamChannelVolume(int index);
int SND_Get2DChannelPlaybackRate(int index);
void SND_Set2DChannelPlaybackRate(int index, int rate);
int SND_Get3DChannelPlaybackRate(int index);
void SND_Set3DChannelPlaybackRate(int index, int rate);
int SND_GetStreamChannelPlaybackRate(int index);
void SND_SetStreamChannelPlaybackRate(int index, int rate);
void SND_UpdateStreamChannelReverb(int index);
int SND_Get2DChannelLength(int index);
int SND_Get3DChannelLength(int index);
int SND_GetStreamChannelLength(int index);
void SND_Get2DChannelSaveInfo(int index, snd_save_2D_sample_t *info);
void SND_Get3DChannelSaveInfo(int index, snd_save_3D_sample_t *info);
void SND_GetStreamChannelSaveInfo(int index, snd_save_stream_t *info);
void SND_EndRawSamples(void);
int SND_RawSamplesTime(void);
void SND_RawSamples(int samples, int rate, int width, int s_channels, const byte *data);
int SND_GetSoundFileSize(const void *pSoundFile);
void SND_DriverPreUpdate(int frametime);
void SND_SetRoomtype(int roomtype);
void SND_Update2DChannelReverb(int index);
void SND_Update3DChannelReverb(int index);
void SND_Set3DPosition(int index, const vec_t *org);
static void MSS_SpatializeStream(float *volume, float *pan);
static void MSS_SpatializeStreamImpl(int i, float *volume, float *pan);
void SND_UpdateStreamChannel(int i, int frametime);
void SND_Update2DChannel(int i, int frametime);
void SND_LoadSoundFile(SoundFile *soundFile);
int SND_StartAlias2DSample(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int *pChannel, snd_alias_system_t system);
int SND_StartAlias3DSample(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, const vec_t *org, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int *pChannel, snd_alias_system_t system);
int SND_StartAliasStreamOnChannel(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, const vec_t *org, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int index, snd_alias_system_t system);
void SND_Update3DChannel(int i, int frametime);
void SND_Set2DChannelFromSaveInfo(int index, snd_save_2D_sample_t *info);
Bool SND_InitDriver(void);
void SND_SetStreamChannelFromSaveInfo(int index, snd_save_stream_t *info);
void SND_DriverPostUpdate(int frametime);

static long unsigned int MSS_FileOpenCallback(const char *pszFilename, long unsigned int *phFileHandle)
{

    return ((unsigned int)FS_FOpenFileReadStream(pszFilename, phFileHandle, 1) >> 31) ^ 1;
}

static void MSS_FileCloseCallback(long unsigned int hFileHandle)
{
    FS_FCloseFile((int)hFileHandle);
}

static long int MSS_FileSeekCallback(long unsigned int hFileHandle, long int offset, long unsigned int type)
{
    switch (type) {
    case 0:
        FS_Seek((int)hFileHandle, offset, 2);
        return FS_FTell((int)hFileHandle);
    case 1:
        FS_Seek((int)hFileHandle, offset, 0);
        return FS_FTell((int)hFileHandle);
    case 2:
        FS_Seek((int)hFileHandle, offset, 1);
        return FS_FTell((int)hFileHandle);
    default:
        return 0;
    }
}

static long unsigned int MSS_FileReadCallback(long unsigned int hFileHandle, void *pBuffer, long unsigned int bytes)
{
    return FS_Read(pBuffer, (int)bytes, (int)hFileHandle);
}

void SND_ShutdownDriver(void)
{
    AIL_close_3D_provider(milesGlob.provider_3D);
    AIL_shutdown();
    memset(&milesGlob, 0, sizeof(MssLocal));
}

int SND_GetDriverCPUPercentage(void)
{
    return AIL_digital_CPU_percent(milesGlob.driver_2D);
}

void SND_Stop2DChannel(int index)
{
    snd_local_t *sndGlob;
    AIL_end_sample(SND_2DHandle(index));
    sndGlob = &g_snd;
    sndGlob->chaninfo[index].paused = 0;
    sndGlob->chaninfo[index].startDelay = 0;
}

void SND_Pause2DChannel(int index)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    AIL_stop_sample(SND_2DHandle(index));
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    chaninfo->paused = 1;
}

void SND_Unpause2DChannel(int index, int timeshift)
{
    snd_local_t *sndGlob = &g_snd;
    if (sndGlob->chaninfo[index].startDelay == 0) {
        AIL_resume_sample(SND_2DHandle(index));
    }
    sndGlob->chaninfo[index].endtime += timeshift;
    sndGlob->chaninfo[index].paused = 0;
}

Bool SND_Is2DChannelFree(int index)
{
    snd_local_t *sndGlob = &g_snd;
    snd_channel_info_t *chaninfo = &sndGlob->chaninfo[index];
    if (chaninfo->paused != 0) {
        return 0;
    }
    if (chaninfo->startDelay != 0) {
        return 0;
    }
    if (chaninfo->pAlias0 == NULL && chaninfo->pAlias1 == NULL) {
        return 1;
    }
    return AIL_sample_status(SND_2DHandle(index)) == 2;
}

void SND_Stop3DChannel(int index)
{
    snd_local_t *sndGlob;
    AIL_end_3D_sample(milesGlob.handle_3D[index]);
    sndGlob = &g_snd;
    sndGlob->chaninfo[index].paused = 0;
    sndGlob->chaninfo[index].startDelay = 0;
}

void SND_Pause3DChannel(int index)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    AIL_stop_3D_sample(milesGlob.handle_3D[index]);
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    chaninfo->paused = 1;
}

void SND_Unpause3DChannel(int index, int timeshift)
{
    snd_local_t *sndGlob = &g_snd;
    if (sndGlob->chaninfo[index].startDelay == 0) {
        AIL_resume_3D_sample(milesGlob.handle_3D[index]);
    }
    sndGlob->chaninfo[index].endtime += timeshift;
    sndGlob->chaninfo[index].paused = 0;
}

Bool SND_Is3DChannelFree(int index)
{
    snd_local_t *sndGlob = &g_snd;
    snd_channel_info_t *chaninfo = &sndGlob->chaninfo[index];
    if (chaninfo->paused != 0) {
        return 0;
    }
    if (chaninfo->startDelay != 0) {
        return 0;
    }
    if (chaninfo->pAlias0 == NULL && chaninfo->pAlias1 == NULL) {
        return 1;
    }
    return AIL_3D_sample_status(milesGlob.handle_3D[index]) == 2;
}

void SND_StopStreamChannel(int index)
{
    snd_local_t *sndGlob;
    int streamIdx = index - 0x20;
    AIL_close_stream(milesGlob.handle_stream[streamIdx]);
    milesGlob.handle_stream[streamIdx] = NULL;
    sndGlob = &g_snd;
    sndGlob->chaninfo[index].paused = 0;
    sndGlob->chaninfo[index].startDelay = 0;
}

void SND_PauseStreamChannel(int index)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    AIL_pause_stream(milesGlob.handle_stream[index - 0x20], 1);
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    chaninfo->paused = 1;
}

void SND_UnpauseStreamChannel(int index, int timeshift)
{
    snd_local_t *sndGlob = &g_snd;
    if (sndGlob->chaninfo[index].startDelay == 0) {
        AIL_pause_stream(milesGlob.handle_stream[index - 0x20], 0);
    }
    sndGlob->chaninfo[index].endtime += timeshift;
    sndGlob->chaninfo[index].paused = 0;
}

Bool SND_IsStreamChannelFree(int index)
{
    snd_local_t *sndGlob;
    void *streamHandle;
    int streamIdx = index - 0x20;
    streamHandle = milesGlob.handle_stream[streamIdx];
    if (streamHandle == NULL) {
        return 1;
    }
    sndGlob = &g_snd;
    if (sndGlob->chaninfo[index].paused != 0) {
        return 0;
    }
    if (sndGlob->chaninfo[index].startDelay != 0) {
        return 0;
    }
    if (sndGlob->chaninfo[index].pAlias0 == NULL && sndGlob->chaninfo[index].pAlias1 == NULL) {
        return 1;
    }
    if (AIL_stream_status(streamHandle) == 2) {
        AIL_close_stream(milesGlob.handle_stream[streamIdx]);
        milesGlob.handle_stream[streamIdx] = NULL;
        return 1;
    }
    return 0;
}

float SND_Get2DChannelVolume(int index)
{
    float left, right;
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    AIL_sample_volume_levels(SND_2DHandle(index), &left, &right);
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    if (chaninfo->srcChannelCount == 2) {
        return left;
    }
    return left + right;
}

float SND_Get3DChannelVolume(int index)
{
    return AIL_3D_sample_volume(milesGlob.handle_3D[index]);
}

float SND_GetStreamChannelVolume(int index)
{
    float left, right;
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    const snd_alias_t *pAlias;
    int channel;
    AIL_stream_volume_levels(milesGlob.handle_stream[index - 0x20], &left, &right);
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    if (chaninfo->srcChannelCount == 2) {
        pAlias = chaninfo->pAlias0;
        channel = (pAlias->flags & 0x780) >> 7;
        if (!SND_IsAliasChannel3D(channel)) {
            return left;
        }
    }
    return left + right;
}

int SND_Get2DChannelPlaybackRate(int index)
{
    return AIL_sample_playback_rate(SND_2DHandle(index));
}

void SND_Set2DChannelPlaybackRate(int index, int rate)
{
    AIL_set_sample_playback_rate(SND_2DHandle(index), rate);
}

int SND_Get3DChannelPlaybackRate(int index)
{
    return AIL_3D_sample_playback_rate(milesGlob.handle_3D[index]);
}

void SND_Set3DChannelPlaybackRate(int index, int rate)
{
    AIL_set_3D_sample_playback_rate(milesGlob.handle_3D[index], rate);
}

int SND_GetStreamChannelPlaybackRate(int index)
{
    return AIL_stream_playback_rate(milesGlob.handle_stream[index - 0x20]);
}

void SND_SetStreamChannelPlaybackRate(int index, int rate)
{
    AIL_set_stream_playback_rate(milesGlob.handle_stream[index - 0x20], rate);
}

void SND_UpdateStreamChannelReverb(int index)
{
    snd_local_t *sndGlob = &g_snd;
    snd_channel_info_t *chaninfo = &sndGlob->chaninfo[index];
    const snd_alias_t *pAlias = chaninfo->pAlias0;
    float reverbLevel;
    if (pAlias != NULL &&
        ((*(const dvar_t **)imp_snd_enableReverb)->current.enabled == 0 || (pAlias->flags & 0x10))) {
        reverbLevel = 0.0f;
    } else {
        reverbLevel = sndGlob->effect->wetlevel;
    }
    AIL_set_stream_reverb_levels(milesGlob.handle_stream[index - 0x20], 1.0f, reverbLevel);
}

int SND_Get2DChannelLength(int index)
{
    long int length;
    long int *lenp = &length;
    AIL_sample_ms_position(SND_2DHandle(index), lenp, NULL);
    return (int)length;
}

int SND_Get3DChannelLength(int index)
{
    return AIL_3D_sample_length(milesGlob.handle_3D[index]);
}

int SND_GetStreamChannelLength(int index)
{
    long int length;
    long int *lenp = &length;
    AIL_stream_ms_position(milesGlob.handle_stream[index - 0x20], lenp, NULL);
    return (int)length;
}

void SND_Get2DChannelSaveInfo(int index, snd_save_2D_sample_t *info)
{
    void *handle;
    long int length, offset;
    snd_local_t *sndGlob;
    float masterVol;

    handle = SND_2DHandle(index);
    AIL_sample_ms_position(handle, &length, &offset);
    info->fraction = (float)(unsigned long int)offset / (float)(unsigned long int)length;
    sndGlob = &g_snd;
    info->pitch = sndGlob->chaninfo[index].pitch;
    AIL_sample_volume_pan(handle, &info->volume, &info->pan);
    masterVol = sndGlob->volume;
    if (masterVol != 0.0f) {
        info->volume /= masterVol;
    } else {
        info->volume = sndGlob->chaninfo[index].basevolume;
    }
}

void SND_Get3DChannelSaveInfo(int index, snd_save_3D_sample_t *info)
{
    void *handle;
    long unsigned int sampleOffset;
    int sampleLength;
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    float vol, masterVol;

    handle = milesGlob.handle_3D[index];
    sampleOffset = AIL_3D_sample_offset(handle);
    sampleLength = AIL_3D_sample_length(handle);
    info->fraction = (float)sampleOffset / (float)sampleLength;
    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[index];
    info->pitch = chaninfo->pitch;
    vol = AIL_3D_sample_volume(handle);
    info->volume = vol;
    masterVol = sndGlob->volume;
    if (masterVol != 0.0f) {
        info->volume = vol / masterVol;
    } else {
        info->volume = chaninfo->basevolume;
    }
    AIL_3D_position(handle, &info->org[0], &info->org[1], &info->org[2]);
}

void SND_GetStreamChannelSaveInfo(int index, snd_save_stream_t *info)
{
    void *handle;
    long int offset, length;
    snd_local_t *sndGlob;
    float masterVol;

    handle = milesGlob.handle_stream[index - 0x20];
    AIL_stream_ms_position(handle, &length, &offset);
    info->fraction = (float)(unsigned long int)offset / (float)(unsigned long int)length;
    sndGlob = &g_snd;
    info->rate = (int)floorf((float)AIL_stream_playback_rate(handle) / sndGlob->timescale + 0.5f);
    info->basevolume = sndGlob->chaninfo[index].basevolume;
    AIL_stream_volume_pan(handle, &info->volume, &info->pan);
    masterVol = sndGlob->volume;
    if (masterVol != 0.0f) {
        info->volume /= masterVol;
    } else {
        info->volume = sndGlob->chaninfo[index].basevolume;
    }
    info->org[0] = sndGlob->chaninfo[index].org[0];
    info->org[1] = sndGlob->chaninfo[index].org[1];
    info->org[2] = sndGlob->chaninfo[index].org[2];
}

void SND_EndRawSamples(void)
{
    if (milesGlob.raw.handle != NULL) {
        AIL_end_sample(milesGlob.raw.handle);
        AIL_release_sample_handle(milesGlob.raw.handle);
        milesGlob.raw.handle = NULL;
        Z_FreeInternal(milesGlob.raw.buf);
    }
}

int SND_RawSamplesTime(void)
{
    if (milesGlob.raw.handle != NULL) {
        long unsigned int pos = AIL_sample_position(milesGlob.raw.handle);
        double dpos = (double)(unsigned long long)pos;
        return (int)(dpos * milesGlob.raw.bufRate + milesGlob.raw.sampleTime);
    }
    return 0;
}

void SND_RawSamples(int samples, int rate, int width, int s_channels, const byte *data)
{
    int bytes, copy, format, bufSize;
    float vol;
    snd_local_t *sndGlob;

    sndGlob = &g_snd;
    if (sndGlob == NULL || !sndGlob->Initialized2d) {
        return;
    }

    if (milesGlob.raw.handle == NULL) {

        milesGlob.raw.handle = AIL_allocate_sample_handle(milesGlob.driver_2D);
        if (milesGlob.raw.handle == NULL) {
            Com_Error(1, (const char *)"\x15MILES 2D sound sample allocation failed on raw channel\n");
        }
        milesGlob.raw.rate = rate;
        milesGlob.raw.width = width;
        milesGlob.raw.channels = s_channels;
        AIL_init_sample(milesGlob.raw.handle);

        if (s_channels == 1) {
            if (width == 1) {
                format = 0;
            } else if (width == 2) {
                format = 1;
            } else {
                format = 0;
            }
        } else {
            if (width == 1) {
                format = 2;
            } else if (width == 2) {
                format = 3;
            } else {
                format = 0;
            }
        }
        AIL_set_sample_type(milesGlob.raw.handle, format, 0);
        AIL_set_sample_playback_rate(milesGlob.raw.handle, rate);
        vol = 0.5f * sndGlob->volume;
        AIL_set_sample_volume_levels(milesGlob.raw.handle, vol, vol);
        bufSize = AIL_minimum_sample_buffer_size(milesGlob.driver_2D, rate, format);
        if (bufSize < 0x2001) {
            bufSize = 0x2000;
        }
        milesGlob.raw.bufSize = bufSize;
        milesGlob.raw.buf = Z_MallocInternal(bufSize * 32);
        memset(milesGlob.raw.bufReady, 0, 32);
        milesGlob.raw.readBuf = 0;
        milesGlob.raw.writeBuf = 0;
        milesGlob.raw.bufUsed = 0;
        milesGlob.raw.sampleTime = 0.0;
        {
            float bytesPerMs = 1000.0f / (float)(milesGlob.raw.rate * milesGlob.raw.width * milesGlob.raw.channels);
            milesGlob.raw.bufRate = (double)bytesPerMs;
        }
    }

    if (milesGlob.raw.handle == NULL) {
        return;
    }

    bytes = width * samples * s_channels;
    while (bytes != 0) {
        while (milesGlob.raw.bufReady[milesGlob.raw.writeBuf] != 0) {
            SND_Update();
        }
        copy = milesGlob.raw.bufSize - milesGlob.raw.bufUsed;
        if (bytes < copy) {
            copy = bytes;
            bytes = 0;
        } else {
            bytes -= copy;
        }
        Com_Memcpy((void *)(milesGlob.raw.buf + milesGlob.raw.writeBuf * milesGlob.raw.bufSize + milesGlob.raw.bufUsed), (void *)data, copy);
        data += copy;
        milesGlob.raw.bufUsed += copy;
        if (milesGlob.raw.bufUsed == milesGlob.raw.bufSize) {
            milesGlob.raw.bufUsed = 0;
            milesGlob.raw.bufReady[milesGlob.raw.writeBuf] = 1;
            milesGlob.raw.writeBuf = (milesGlob.raw.writeBuf + 1) % 32;
        }
    }
}

int SND_GetSoundFileSize(const void *pSoundFile)
{
    const MssSound *sound = (const MssSound *)pSoundFile;
    return (int)sound->info.data_len + 0x24;
}

void SND_DriverPreUpdate(int frametime)
{
}

void SND_SetRoomtype(int roomtype)
{
    snd_local_t *sndGlob;
    float reverbLevel;
    AIL_set_digital_master_room_type(milesGlob.driver_2D, roomtype);
    sndGlob = &g_snd;
    reverbLevel = sndGlob->effect->wetlevel;
    AIL_set_digital_master_reverb_levels(milesGlob.driver_2D, 1.0f, reverbLevel);
    AIL_set_3D_room_type(milesGlob.provider_3D, roomtype);
}

void SND_Update2DChannelReverb(int index)
{
    snd_local_t *sndGlob = &g_snd;
    snd_channel_info_t *chaninfo = &sndGlob->chaninfo[index];
    const snd_alias_t *pAlias = chaninfo->pAlias0;
    float reverbLevel;

    if (pAlias == NULL || ((*(dvar_t **)imp_snd_enableReverb)->current.enabled != 0 && !(pAlias->flags & 0x10))) {
        reverbLevel = 0;
    } else {
        reverbLevel = sndGlob->effect->wetlevel;
    }
    AIL_set_sample_reverb_levels(SND_2DHandle(index), 1.0f, reverbLevel);
}

void SND_Update3DChannelReverb(int index)
{
    snd_local_t *sndGlob = &g_snd;
    snd_channel_info_t *chaninfo = &sndGlob->chaninfo[index];
    const snd_alias_t *pAlias = chaninfo->pAlias0;
    float reverbLevel;

    if (pAlias == NULL || ((*(dvar_t **)imp_snd_enableReverb)->current.enabled != 0 && !(pAlias->flags & 0x10))) {
        reverbLevel = 0;
    } else {
        reverbLevel = sndGlob->effect->wetlevel;
    }
    AIL_set_3D_sample_effects_level(milesGlob.handle_3D[index], reverbLevel);
}

void SND_Set3DPosition(int index, const vec_t *org)
{
    int listenerIdx;
    snd_local_t *sndGlob;
    snd_listener *listener;
    float *listenerOrigin, *right, *up, *forward;
    float dx, dy, dz;
    float x, y, z;

    listenerIdx = SND_GetListenerIndexNearestToOrigin(org);
    sndGlob = &g_snd;
    listener = &sndGlob->listeners[listenerIdx];
    listenerOrigin = listener->orient.origin;
    right = listener->orient.axis[0];
    up = listener->orient.axis[1];
    forward = listener->orient.axis[2];

    dx = org[0] - listenerOrigin[0];
    dy = org[1] - listenerOrigin[1];
    dz = org[2] - listenerOrigin[2];

    x = dx * right[0] + dy * right[1] + dz * right[2];
    z = dx * forward[0] + dy * forward[1] + dz * forward[2];
    y = -(dx * up[0] + dy * up[1] + dz * up[2]);

    AIL_set_3D_position(milesGlob.handle_3D[index], x, y, z);
}

static void MSS_SpatializeStream(float *volume, float *pan)
{

}

static void MSS_SpatializeStreamImpl(int i, float *volume, float *pan)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    const snd_alias_t *pAlias0, *pAlias1;
    float *orgVec;
    int listenerIdx;
    snd_listener *listener;
    float *listenerOrigin, *right;
    float delta[3];
    float dot;
    float lerp, oneMinusLerp;
    float fDistMin, fDistMax;
    float savedVolume, attenuation, distance;

    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[i];
    pAlias0 = chaninfo->pAlias0;
    pAlias1 = chaninfo->pAlias1;

    orgVec = chaninfo->org;

    listenerIdx = SND_GetListenerIndexNearestToOrigin(orgVec);
    listener = &sndGlob->listeners[listenerIdx];
    listenerOrigin = listener->orient.origin;

    delta[0] = orgVec[0] - listenerOrigin[0];
    delta[1] = orgVec[1] - listenerOrigin[1];
    delta[2] = orgVec[2] - listenerOrigin[2];

    distance = Vec3Normalize(delta);

    /* Engine axis[1] points left; pan zero is left and one is right. */
    right = listener->orient.axis[1];
    dot = delta[0] * right[0] + delta[1] * right[1] + delta[2] * right[2];

    lerp = chaninfo->lerp;
    oneMinusLerp = 1.0f - lerp;

    fDistMin = oneMinusLerp * pAlias0->fDistMin + lerp * pAlias1->fDistMin;
    fDistMax = oneMinusLerp * pAlias0->fDistMax + lerp * pAlias1->fDistMax;

    savedVolume = *volume;
    attenuation = SND_Attenuate(pAlias0->volumeFalloffCurve, distance, fDistMin, fDistMax);
    *volume = savedVolume * attenuation;
    *pan = (1.0f - dot) * 0.5f;
}

void SND_UpdateStreamChannel(int i, int frametime)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    const snd_alias_t *pAlias0;
    int streamIdx;
    float volume, pan;
    int aliasFlags, channel;
    float *volTable;
    float scaledVol;
    int startDelay;

    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[i];

    if (chaninfo->paused != 0) {
        return;
    }

    if (i <= 0x22) {
        if (!SND_UpdateBackgroundVolume(i - 0x20, frametime)) {
            return;
        }
    }

    pAlias0 = chaninfo->pAlias0;
    if (pAlias0 == NULL || chaninfo->pAlias1 == NULL) {
        return;
    }

    volume = chaninfo->basevolume;
    pan = 0.5f;

    aliasFlags = pAlias0->flags;
    channel = (aliasFlags & 0x780) >> 7;

    if (SND_IsAliasChannel3D(channel)) {

        streamIdx = i - 0x20;
        SND_GetCurrent3DPosition(chaninfo->entnum, chaninfo->offset, chaninfo->org);
        MSS_SpatializeStreamImpl(i, &volume, &pan);

        if (AIL_is_3D_stream(milesGlob.handle_stream[streamIdx])) {

            int listenerIdx = SND_GetListenerIndexNearestToOrigin(chaninfo->org);
            snd_listener *listener = &sndGlob->listeners[listenerIdx];
            float *listenerOrigin = listener->orient.origin;
            float *right = listener->orient.axis[0];
            float *up = listener->orient.axis[1];
            float *forward = listener->orient.axis[2];
            float dx = chaninfo->org[0] - listenerOrigin[0];
            float dy = chaninfo->org[1] - listenerOrigin[1];
            float dz = chaninfo->org[2] - listenerOrigin[2];
            float x = dx * right[0] + dy * right[1] + dz * right[2];
            float z = dx * forward[0] + dy * forward[1] + dz * forward[2];
            float y = -(dx * up[0] + dy * up[1] + dz * up[2]);
            AIL_set_3D_stream_position(milesGlob.handle_stream[streamIdx], y, z, x);
            pan = 0.5f;
        }
    } else {
        streamIdx = i - 0x20;
    }

    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[i];
    if (sndGlob->slaveLerp != 0.0f && chaninfo->master == 0) {
        aliasFlags = pAlias0->flags;
        if (aliasFlags & 4) {
            volume *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
        }
        aliasFlags = pAlias0->flags;
    } else {
        aliasFlags = pAlias0->flags;
    }

    volTable = (float *)sndGlob->channelvol;
    channel = (aliasFlags & 0x780) >> 7;
    volume *= volTable[channel * 3];
    pan = pan;
    scaledVol = volume * sndGlob->volume;

    if (chaninfo->srcChannelCount == 2) {

        const snd_alias_t *pA = chaninfo->pAlias0;
        int ch3d = (pA->flags & 0x780) >> 7;
        if (!SND_IsAliasChannel3D(ch3d)) {
            AIL_set_stream_volume_levels(milesGlob.handle_stream[streamIdx], scaledVol, scaledVol);
        } else {
            float leftVol = pan * scaledVol;
            float rightVol = (1.0f - pan) * scaledVol;
            AIL_set_stream_volume_levels(milesGlob.handle_stream[streamIdx], rightVol, leftVol);
        }
    } else {
        float leftVol = pan * scaledVol;
        float rightVol = (1.0f - pan) * scaledVol;
        AIL_set_stream_volume_levels(milesGlob.handle_stream[streamIdx], rightVol, leftVol);
    }

    startDelay = chaninfo->startDelay;
    if (startDelay != 0) {
        int newDelay = startDelay - frametime;
        if (frametime >= startDelay) {
            newDelay = 0;
        }
        chaninfo->startDelay = newDelay;
        if (newDelay == 0) {
            AIL_pause_stream(milesGlob.handle_stream[streamIdx], 0);
        }
    }
}

void SND_Update2DChannel(int i, int frametime)
{
    snd_local_t *sndGlob;
    snd_channel_info_t *chaninfo;
    const snd_alias_t *pAlias0;
    int aliasFlags, channel;
    float volume, scaledVol;
    float *volTable;
    int startDelay, newDelay;

    sndGlob = &g_snd;
    chaninfo = &sndGlob->chaninfo[i];

    if (chaninfo->paused != 0) {
        return;
    }

    pAlias0 = chaninfo->pAlias0;
    if (pAlias0 == NULL || chaninfo->pAlias1 == NULL) {
        return;
    }

    volume = chaninfo->basevolume;

    if (sndGlob->slaveLerp != 0.0f) {
        if (chaninfo->master == 0) {
            aliasFlags = pAlias0->flags;
            if (aliasFlags & 4) {
                float slavePct = SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
                volume *= slavePct;
            }
            aliasFlags = pAlias0->flags;
        } else {
            aliasFlags = pAlias0->flags;
        }
    } else {
        aliasFlags = pAlias0->flags;
    }

    volTable = (float *)sndGlob->channelvol;
    channel = (aliasFlags & 0x780) >> 7;
    scaledVol = volume * volTable[channel * 3] * sndGlob->volume;

    if (chaninfo->srcChannelCount != 2) {
        scaledVol *= 0.5f;
    }
    AIL_set_sample_volume_levels(SND_2DHandle(i), scaledVol, scaledVol);

    startDelay = chaninfo->startDelay;
    if (startDelay != 0) {
        newDelay = startDelay - frametime;
        if (frametime >= startDelay) {
            newDelay = 0;
        }
        chaninfo->startDelay = newDelay;
        if (newDelay == 0) {
            return AIL_resume_sample(SND_2DHandle(i));
        }
    }
}

void SND_LoadSoundFile(SoundFile *soundFile)
{
    snd_local_t *sndGlob;
    char realname[256];
    void *buffer;
    AILMIXINFO mixinfo;
    int datasize, totalSize, rate;
    MssSound *sound;
    int targetBits, targetChannels;
    int procFormat;
    int fileLength;

    sndGlob = &g_snd;
    if (!sndGlob->Initialized2d) {
        soundFile->fileMem = NULL;
        return;
    }

    sprintf(realname, (const char *)"sound/%s", soundFile->soundName);
    if ((fileLength = FS_ReadFile(realname, &buffer)) < 0) {
        Com_Printf((const char *)"^1ERROR: Sound file '%s' not found\n", realname);
        soundFile->fileMem = NULL;
        return;
    }

#ifdef __EMSCRIPTEN__
    extern int WebAudio_WavInfo(const void *, int, AILSOUNDINFO *);
    if (!WebAudio_WavInfo(buffer, fileLength, &mixinfo.Info)) {
#else
    if (!AIL_WAV_info(buffer, &mixinfo)) {
#endif
        Com_Printf((const char *)"^1ERROR: Sound file '%s' is in an invalid or corrupted format\n", realname);
        sound = NULL;
    } else if ((totalSize = (int)mixinfo.Info.data_len + 0x24) == 0) {
        Com_Printf((const char *)"^1ERROR: Sound file '%s' is zero length, invalid\n", realname);
        sound = NULL;
    } else if (
#ifdef __EMSCRIPTEN__
               1 /* Browser output resamples the original PCM at playback. */
#else
               mixinfo.Info.rate <= (unsigned long int)sndGlob->playback_rate &&
               ((long int)mixinfo.Info.bits <= sndGlob->playback_bits || mixinfo.Info.format == 0x11) &&
               (long int)mixinfo.Info.channels <= sndGlob->playback_channels
#endif
    ) {

        sound = (MssSound *)Hunk_AllocNoZeroInternal(totalSize);
        Com_Memcpy(sound->data, (void *)mixinfo.Info.data_ptr, mixinfo.Info.data_len);
        sound->info.format = mixinfo.Info.format;
        sound->info.data_ptr = mixinfo.Info.data_ptr;
        sound->info.data_len = mixinfo.Info.data_len;
        sound->info.rate = mixinfo.Info.rate;
        sound->info.bits = mixinfo.Info.bits;
        sound->info.channels = mixinfo.Info.channels;
        sound->info.samples = mixinfo.Info.samples;
        sound->info.block_size = mixinfo.Info.block_size;
        sound->info.data_ptr = sound->data;
        sound->info.initial_ptr = sound->data;
    } else {

        rate = (int)mixinfo.Info.rate;
        while ((unsigned int)rate > (unsigned int)sndGlob->playback_rate) {
            rate >>= 1;
            mixinfo.Info.samples >>= 1;
        }

        targetBits = (int)mixinfo.Info.bits;
        if (targetBits >= sndGlob->playback_bits) {
            targetBits = sndGlob->playback_bits;
        }
        targetChannels = (int)mixinfo.Info.channels;
        if (targetChannels >= sndGlob->playback_channels) {
            targetChannels = sndGlob->playback_channels;
        }

        if (targetChannels - 1 == 0) {

            if (mixinfo.Info.format == 0x11) {
                procFormat = 5;
            } else if (targetBits > 8) {
                procFormat = 8;
                if (targetBits <= 16) {
                    procFormat = targetChannels;
                }
            } else {
                procFormat = 0;
            }
        } else {

            if (mixinfo.Info.format == 0x11) {
                procFormat = 7;
            } else if (targetBits > 8) {
                procFormat = 0xa;
                if (targetBits <= 16) {
                    procFormat = 3;
                }
            } else {
                procFormat = 2;
            }
        }

        datasize = AIL_size_processed_digital_audio(rate, procFormat, 1, &mixinfo);
        sound = (MssSound *)Hunk_AllocNoZeroInternal(datasize + 0x24);

        sound->info.format = mixinfo.Info.format;
        sound->info.data_ptr = sound->data;
        sound->info.data_len = datasize;
        sound->info.rate = rate;
        sound->info.bits = targetBits;
        sound->info.channels = targetChannels;
        sound->info.samples = mixinfo.Info.samples;
        sound->info.block_size = mixinfo.Info.block_size;
        sound->info.initial_ptr = sound->data;

        AIL_process_digital_audio(sound->data, datasize, rate, procFormat, 1, &mixinfo);
    }

    soundFile->fileMem = sound;
    FS_FreeFile(buffer);
}

int SND_StartAlias2DSample(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int *pChannel, snd_alias_system_t system)
{
    int index, channel, aliasFlags, sampleType;
    void *handle;
    byte *sound;
    snd_local_t *sndGlob;
    float scaledVol, volMul;
    float *volTable;
    long int total_msec;
    int start_msec;
    int bits, channels, format;
    float reverbLevel;

    aliasFlags = pAlias0->flags;
    channel = (aliasFlags & 0x780) >> 7;
    index = SND_FindFree2DChannel(entnum, channel);
    if (pChannel != NULL) {
        *pChannel = index;
    }
    if (index < 0) {
        return 0;
    }

    handle = SND_2DHandle(index);
    sound = (byte *)Com_GetSoundFileMem(pAlias0);
    AIL_init_sample(handle);

    bits = (int)((MssSound *)sound)->info.bits;
    channels = (int)((MssSound *)sound)->info.channels;
    format = (int)((MssSound *)sound)->info.format;

    if (channels == 1) {
        if (format == 0x11)
            sampleType = 5;
        else if (bits <= 8)
            sampleType = 0;
        else if (bits <= 16)
            sampleType = 1;
        else
            sampleType = 8;
    } else {
        if (format == 0x11)
            sampleType = 7;
        else if (bits <= 8)
            sampleType = 2;
        else if (bits <= 16)
            sampleType = 3;
        else
            sampleType = 0xa;
    }

#ifdef __EMSCRIPTEN__
    AIL_set_3D_sample_info(handle, &((MssSound *)sound)->info);
#else
    AIL_set_sample_type(handle, sampleType, 0);
    AIL_set_sample_address(handle, (void *)((MssSound *)sound)->info.data_ptr, (int)((MssSound *)sound)->info.data_len);
    AIL_set_sample_adpcm_block_size(handle, (int)((MssSound *)sound)->info.block_size);
#endif

    sndGlob = &g_snd;
    AIL_set_sample_playback_rate(handle, (int)floorf((float)(int)((MssSound *)sound)->info.rate * pitch * sndGlob->timescale + 0.5f));

    aliasFlags = pAlias0->flags;
    volTable = (float *)sndGlob->channelvol;
    channel = (aliasFlags & 0x780) >> 7;
    scaledVol = volume * sndGlob->volume * volTable[channel * 3];

    if (sndGlob->slaveLerp != 0.0f && !master && (aliasFlags & 4)) {
        scaledVol *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
    }

    volMul = (channels == 1) ? 0.5f : 1.0f;
    AIL_set_sample_volume_levels(handle, volMul * scaledVol, volMul * scaledVol);

    AIL_set_sample_loop_count(handle, ((byte)(pAlias0->flags) & 1) ^ 1);

    if ((*(dvar_t **)imp_snd_enableReverb)->current.enabled != 0 && !((byte)(pAlias0->flags) & 0x10)) {
        reverbLevel = g_snd.effect->wetlevel;
    } else {
        reverbLevel = 0;
    }
    AIL_set_sample_reverb_levels(handle, 1.0f, reverbLevel);

    AIL_sample_ms_position(handle, &total_msec, NULL);
    if (total_msec <= timeshift) {
        return 0;
    }

    if (fraction != 0.0f) {
        start_msec = (int)floorf((float)total_msec * fraction + 0.5f);
    } else {
        start_msec = timeshift;
    }

    if (start_msec == 0) {
        startDelay = startDelay;
    } else {
        startDelay = 0;
    }

    AIL_set_sample_ms_position(handle, start_msec);

    if (startDelay == 0) {
        sndGlob = &g_snd;
        aliasFlags = pAlias0->flags;
        channel = (aliasFlags & 0x780) >> 7;
        if (sndGlob->paused == 0 || sndGlob->pauseSettings[channel] == 0) {
            AIL_resume_sample(handle);
        }
        aliasFlags = pAlias0->flags;
    } else {
        aliasFlags = pAlias0->flags;
    }

    total_msec += startDelay;
    if (aliasFlags & 1) {
        total_msec = 0;
    }

    SND_SetChannelInfo(index, entnum, pAlias0, pAlias1, lerp, NULL, volume, pitch, channels, (int)((MssSound *)sound)->info.rate, total_msec, start_msec, startDelay, master, system);
    return (int)total_msec;
}

int SND_StartAlias3DSample(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, const vec_t *org, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int *pChannel, snd_alias_system_t system)
{
    int index, channel, aliasFlags;
    void *handle;
    byte *sound;
    snd_local_t *sndGlob;
    float scaledVol;
    float *volTable;
    float fDistMin, fDistMax, oneMinusLerp;
    float dist, attenuation;
    int playbackRate, total_msec, start_msec;
    int soundSamples;

    aliasFlags = pAlias0->flags;
    channel = (aliasFlags & 0x780) >> 7;
    index = SND_FindFree3DChannel(entnum, channel);
    if (pChannel != NULL) {
        *pChannel = index;
    }
    if (index < 0) {
        return 0;
    }

    handle = milesGlob.handle_3D[index];
    sound = (byte *)Com_GetSoundFileMem(pAlias0);

    oneMinusLerp = 1.0f - lerp;
    fDistMin = oneMinusLerp * pAlias0->fDistMin + lerp * pAlias1->fDistMin;
    fDistMax = oneMinusLerp * pAlias0->fDistMax + lerp * pAlias1->fDistMax;

    AIL_set_3D_sample_info(handle, sound);

    {
        int listenerIdx = SND_GetListenerIndexNearestToOrigin(org);
        sndGlob = &g_snd;
        dist = Vec3Distance(org, sndGlob->listeners[listenerIdx].orient.origin);
    }
    attenuation = SND_Attenuate(pAlias0->volumeFalloffCurve, dist, fDistMin, fDistMax);

    aliasFlags = pAlias0->flags;
    sndGlob = &g_snd;
    volTable = (float *)sndGlob->channelvol;
    channel = (aliasFlags & 0x780) >> 7;
    scaledVol = attenuation * volume * volTable[channel * 3] * sndGlob->volume;

    if (sndGlob->slaveLerp != 0.0f && !master && (aliasFlags & 4)) {
        scaledVol *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
    }

    AIL_set_3D_sample_volume(handle, scaledVol);
    AIL_set_3D_sample_distances(handle, pAlias0->fDistMin, pAlias0->fDistMax);

    playbackRate = AIL_3D_sample_playback_rate(handle);
    sndGlob = &g_snd;
    AIL_set_3D_sample_playback_rate(handle, (int)floorf((float)playbackRate * pitch * sndGlob->timescale + 0.5f));
    SND_Set3DPosition(index, org);

    AIL_set_3D_sample_loop_count(handle, ((byte)(pAlias0->flags) & 1) ^ 1);
    AIL_set_3D_sample_effects_level(handle, sndGlob->effect->wetlevel);

    soundSamples = (int)((MssSound *)sound)->info.samples;
    {
        int val = soundSamples * 5 * 5 * 5 * 8;
        float totalFloat = (float)val * g_snd.timescale / (float)playbackRate + 0.5f;
        total_msec = (int)floorf(totalFloat);
    }

    if (total_msec <= timeshift) {
        return 0;
    }

    if (fraction != 0.0f) {
        start_msec = (int)floorf((float)total_msec * fraction + 0.5f);
    } else {
        start_msec = timeshift;
    }

    if (start_msec == 0) {

    } else {
        startDelay = 0;
    }

    {
        int datasize = (int)((MssSound *)sound)->info.data_len;
        float bytePos = (float)datasize * fraction + 0.5f;
        AIL_set_3D_sample_offset(handle, (int)floorf(bytePos));
    }

    if (startDelay == 0) {
        sndGlob = &g_snd;
        aliasFlags = pAlias0->flags;
        channel = (aliasFlags & 0x780) >> 7;
        if (sndGlob->paused == 0 || sndGlob->pauseSettings[channel] == 0) {
            AIL_resume_3D_sample(handle);
        }
        aliasFlags = pAlias0->flags;
    } else {
        aliasFlags = pAlias0->flags;
    }

    total_msec += startDelay;
    if (aliasFlags & 1) {
        total_msec = 0;
    }

    SND_SetChannelInfo(index, entnum, pAlias0, pAlias1, lerp, org, volume, pitch, (int)((MssSound *)sound)->info.channels, (int)((MssSound *)sound)->info.rate, total_msec, start_msec, startDelay, master, system);
    return total_msec;
}

int SND_StartAliasStreamOnChannel(const snd_alias_t *pAlias0, const snd_alias_t *pAlias1, float lerp, int entnum, const vec_t *org, float volume, float pitch, int timeshift, float fraction, int startDelay, int master, int index, snd_alias_system_t system)
{
    int streamIdx;
    void *handle;
    char realname[256];
    int filetype;
    int srcChannelCount;
    int baserate;
    snd_local_t *sndGlob;
    int aliasFlags, channel;
    float *volTable;
    float scaledVol;
    float pan;
    float reverbLevel;
    long int total_msec;
    int start_msec;
    byte *chanBase;
    float *to;

    if (pAlias0->soundFile->isStreamFound == 0) {
        Com_DPrintf((const char *)"Tried to play streamed sound '%s' from alias '%s', but it was not found at load time.\n", Com_GetSoundFileName(pAlias0), pAlias0->pszAliasName);
        return 0;
    }

    streamIdx = index - 0x20;
    if (milesGlob.handle_stream[streamIdx] != NULL) {
        AIL_close_stream(milesGlob.handle_stream[streamIdx]);
        milesGlob.handle_stream[streamIdx] = NULL;
    }

    sprintf(realname, (const char *)"sound/%s", Com_GetSoundFileName(pAlias0));

    if (mss_q3fs->current.enabled != 0) {
        handle = AIL_open_stream(milesGlob.driver_2D, realname, 0);
        if (handle != NULL) {
            goto got_handle;
        }

    }

    handle = AIL_open_stream(milesGlob.driver_2D, FS_ShortOSFilePath(realname), 0);
    if (handle == NULL) {
        Com_Printf((const char *)"Couldn't play stream '%s' from alias '%s' - %s\n", realname, pAlias0->pszAliasName, AIL_last_error());
        return 0;
    }

got_handle:

    milesGlob.handle_stream[streamIdx] = handle;

    AIL_stream_info(handle, &filetype, NULL, NULL, NULL);
    srcChannelCount = (filetype & 2) ? 2 : 1;

    baserate = AIL_stream_playback_rate(handle);
    sndGlob = &g_snd;
    AIL_set_stream_playback_rate(handle, (int)floorf((float)baserate * pitch * sndGlob->timescale + 0.5f));

    aliasFlags = pAlias0->flags;
    scaledVol = sndGlob->volume * volume;
    volTable = (float *)sndGlob->channelvol;
    channel = (aliasFlags & 0x780) >> 7;
    scaledVol *= volTable[channel * 3];

    if (sndGlob->slaveLerp != 0.0f && !master) {
        if (aliasFlags & 4) {
            scaledVol *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
        }
    }

    if (srcChannelCount == 1) {
        pan = 0.5f;
    } else {
        pan = 1.0f;
    }
    AIL_set_stream_volume_levels(handle, pan * scaledVol, pan * scaledVol);

    AIL_set_stream_loop_count(handle, ((byte)(pAlias0->flags) & 1) ^ 1);

    if ((*(dvar_t **)imp_snd_enableReverb)->current.enabled != 0 && !((byte)(pAlias0->flags) & 0x10)) {
        reverbLevel = g_snd.effect->wetlevel;
    } else {
        reverbLevel = 0;
    }
    AIL_set_stream_reverb_levels(handle, 1.0f, reverbLevel);

    AIL_stream_ms_position(handle, &total_msec, NULL);
    if (total_msec <= timeshift) {
        return 0;
    }
    if (total_msec == 0) {
        Com_Printf((const char *)"^1ERROR: Sound file '%s' is zero length, invalid\n", realname);
        return 0;
    }

    if (fraction != 0.0f) {
        start_msec = (int)floorf((float)total_msec * fraction + 0.5f);
    } else {
        start_msec = timeshift;
    }

    if (start_msec != 0) {
        startDelay = 0;
    }

    AIL_set_stream_ms_position(handle, start_msec);

    if (startDelay == 0) {
        sndGlob = &g_snd;
        aliasFlags = pAlias0->flags;
        channel = (aliasFlags & 0x780) >> 7;
        if (sndGlob->paused == 0 || sndGlob->pauseSettings[channel] == 0) {
            AIL_pause_stream(handle, 0);
        }
        aliasFlags = pAlias0->flags;
    } else {
        aliasFlags = pAlias0->flags;
    }

    total_msec += startDelay;
    if (aliasFlags & 1) {
        total_msec = 0;
    }

    {
        sndGlob = &g_snd;
        to = sndGlob->chaninfo[index].org;
        to[0] = org[0];
        to[1] = org[1];
        to[2] = org[2];
    }

    SND_SetChannelInfo(index, entnum, pAlias0, pAlias1, lerp, org, volume, pitch, srcChannelCount, baserate, (int)total_msec, start_msec, startDelay, master, (int)system);

    {
        sndGlob = &g_snd;
        int chFlags = sndGlob->chaninfo[index].pAlias0->flags;
        channel = (chFlags & 0x780) >> 7;
    }

    if (SND_IsAliasChannel3D(channel)) {

        MSS_SpatializeStreamImpl(index, &volume, &pan);

        if (AIL_is_3D_stream(milesGlob.handle_stream[streamIdx])) {
            sndGlob = &g_snd;
            to = sndGlob->chaninfo[index].org;
            {
                int listenerIdx = SND_GetListenerIndexNearestToOrigin(to);
                float *listenerOrigin = sndGlob->listeners[listenerIdx].orient.origin;
                float *right = sndGlob->listeners[listenerIdx].orient.axis[0];
                float *up = sndGlob->listeners[listenerIdx].orient.axis[1];
                float *forward = sndGlob->listeners[listenerIdx].orient.axis[2];
                float dx = to[0] - listenerOrigin[0];
                float dy = to[1] - listenerOrigin[1];
                float dz = to[2] - listenerOrigin[2];
                float x = dx * right[0] + dy * right[1] + dz * right[2];
                float z = dx * forward[0] + dy * forward[1] + dz * forward[2];
                float y = -(dx * up[0] + dy * up[1] + dz * up[2]);
                AIL_set_3D_stream_position(milesGlob.handle_stream[streamIdx], y, z, x);
            }
            pan = 0.5f;
        }

        sndGlob = &g_snd;
        aliasFlags = pAlias0->flags;
        scaledVol = sndGlob->volume * volume;
        volTable = (float *)sndGlob->channelvol;
        channel = (aliasFlags & 0x780) >> 7;
        scaledVol *= volTable[channel * 3];

        if (sndGlob->slaveLerp != 0.0f && !master && (aliasFlags & 4)) {
            scaledVol *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
        }

        {
            float leftVol = pan * scaledVol;
            float rightVol = (1.0f - pan) * scaledVol;
            AIL_set_stream_volume_levels(handle, rightVol, leftVol);
        }
    }

    return (int)total_msec;
}

void SND_Update3DChannel(int i, int frametime)
{
    snd_local_t *sndGlob;
    byte *ch;
    const snd_alias_t *pAlias0, *pAlias1;
    float lerp, oneMinusLerp;
    float volume;
    float fDistMin, fDistMax;
    float org[3];
    float dist, attenuation;
    int aliasFlags, channel;
    float *volTable;
    float scaledVol;
    int startDelay, newDelay;

    sndGlob = &g_snd;
    ch = (byte *)&sndGlob->chaninfo[i];

    {
        snd_channel_info_t *ci = &sndGlob->chaninfo[i];

        if (ci->paused != 0) {
            return;
        }

        pAlias0 = ci->pAlias0;
        pAlias1 = ci->pAlias1;
        if (pAlias0 == NULL || pAlias1 == NULL) {
            return;
        }

        lerp = ci->lerp;
        volume = ci->basevolume;

        SND_GetCurrent3DPosition(ci->entnum, ci->offset, org);
        SND_Set3DPosition(i, org);

        oneMinusLerp = 1.0f - lerp;
        fDistMin = oneMinusLerp * pAlias0->fDistMin + lerp * pAlias1->fDistMin;
        fDistMax = oneMinusLerp * pAlias0->fDistMax + lerp * pAlias1->fDistMax;

        {
            int listenerIdx = SND_GetListenerIndexNearestToOrigin(org);
            sndGlob = &g_snd;
            dist = Vec3Distance(org, sndGlob->listeners[listenerIdx].orient.origin);
        }
        attenuation = SND_Attenuate(pAlias0->volumeFalloffCurve, dist, fDistMin, fDistMax);
        volume *= attenuation;

        sndGlob = &g_snd;
        if (sndGlob->slaveLerp != 0.0f && ci->master == 0) {
            aliasFlags = pAlias0->flags;
            if (aliasFlags & 4) {
                volume *= SND_GetLerpedSlavePercentage(pAlias0->fSlavePercentage);
            }
            aliasFlags = pAlias0->flags;
        } else {
            aliasFlags = pAlias0->flags;
        }

        sndGlob = &g_snd;
        volTable = (float *)sndGlob->channelvol;
        channel = (aliasFlags & 0x780) >> 7;
        scaledVol = volume * volTable[channel * 3] * sndGlob->volume;
        AIL_set_3D_sample_volume(milesGlob.handle_3D[i], scaledVol);

        {
            startDelay = sndGlob->chaninfo[i].startDelay;
            if (startDelay != 0) {
                newDelay = startDelay - frametime;
                if (frametime >= startDelay) {
                    newDelay = 0;
                }
                sndGlob->chaninfo[i].startDelay = newDelay;
                if (newDelay == 0) {
                    AIL_resume_3D_sample(milesGlob.handle_3D[i]);
                }
            }
        }
    }
}

void SND_Set2DChannelFromSaveInfo(int index, snd_save_2D_sample_t *info)
{
    float pan = info->pan;
    float vol = info->volume * g_snd.volume;

    if (g_snd.chaninfo[index].srcChannelCount == 2) {
        AIL_set_sample_volume_levels(SND_2DHandle(index), vol, vol);
    } else {
        float leftVol = (1.0f - pan) * vol;
        float rightVol = pan * vol;
        AIL_set_sample_volume_levels(SND_2DHandle(index), leftVol, rightVol);
    }
}

Bool SND_InitDriver(void)
{
    int freq, khz, rate;
    int bits, bytes;
    int numChannels;
    const char *channelStr;
    snd_local_t *sndGlob;
    const char *wantedName;
    void *providerIter;
    void *handle;
    void *defaultHandle;
    void *wantedHandle;
    const char *name;
    int i;
    int sndKhzVal, sndBitsVal;
    unsigned int maxRate;

    mss_q3fs = Dvar_RegisterBool((const char *)"mss_q3fs", 1, 0x1020);
    if (mss_q3fs->current.enabled != 0) {
        AIL_set_file_callbacks((void *)MSS_FileOpenCallback, (void *)MSS_FileCloseCallback, (void *)MSS_FileSeekCallback, (void *)MSS_FileReadCallback);
    }

    AIL_set_redist_directory((const char *)"miles");
    if (!AIL_startup(0x75)) {
        goto startup_failed;
    }

    mss_3d_provider = Dvar_RegisterString((const char *)"mss_3d_provider", (const char *)"Miles Fast 2D Positional Audio", 0x1021);

    sndKhzVal = (*(dvar_t **)imp_snd_khz)->current.integer;
    switch (sndKhzVal) {
    case 0x16:
        freq = 0x5622;
        khz = 0x16;
        rate = 0x5622;
        break;
    case 0x2c:
        freq = 0xac44;
        khz = 0x2c;
        rate = 0xac44;
        break;
    case 0xb:
        freq = 0x2b11;
        khz = 0xb;
        rate = 0x2b11;
        break;
    default:
        Com_Printf((const char *)"invalid value %i for snd_khz, using 22 khz instead\n", sndKhzVal);
        freq = 0x5622;
        khz = 0x16;
        rate = 0x5622;
        break;
    }

    sndBitsVal = (*(dvar_t **)imp_snd_bits)->current.integer;
    if (sndBitsVal == 8) {
        bytes = 1;
        bits = 8;
    } else {
        if (sndBitsVal != 0x10) {
            Com_Printf((const char *)"invalid value %i for snd_bits (should be 8 or 16), using 16 instead\n", sndBitsVal);
        }
        bytes = 2;
        bits = 0x10;
    }

    if ((*(dvar_t **)imp_snd_stereo)->current.enabled != 0) {
        channelStr = (const char *)"stereo";
        numChannels = 2;
    } else {
        channelStr = (const char *)"mono";
        numChannels = 1;
    }

    Com_Printf((const char *)"Attempting %i kHz %i bit %s sound\n", khz, bits, channelStr);

    AIL_set_preference(1, 0x35);
    milesGlob.driver_2D = AIL_open_digital_driver(rate, bytes, numChannels, 0);

    if (milesGlob.driver_2D == NULL) {
        Com_Printf((const char *)"couldn't initialize 2D provider: %s\n", AIL_last_error());
        goto shutdown_and_fail;
    }

    sndGlob = &g_snd;
    sndGlob->Initialized2d = 1;
    sndGlob->max_2D_channels = 8;
    sndGlob->max_stream_channels = 0xd;

    maxRate = ((unsigned int)freq >> 1) + (unsigned int)freq;
    if (maxRate < 0xac44) {
        sndGlob->playback_rate = (int)maxRate;
    } else {
        sndGlob->playback_rate = 0x7fffffff;
    }
    sndGlob->playback_bits = bits;
    sndGlob->playback_channels = numChannels;
    sndGlob->timescale = 1.0f;

    wantedName = mss_3d_provider->current.string;
    Com_Printf((const char *)"available 3D providers:\n");
    providerIter = NULL;
    wantedHandle = NULL;
    defaultHandle = NULL;

    if (AIL_enumerate_3D_providers(&providerIter, &handle, &name)) {
        do {
            Com_Printf((const char *)"  %s\n", name);
            if (stricmp(name, wantedName) == 0) {
                wantedHandle = handle;
            }
            if (stricmp(name, (const char *)"Miles Fast 2D Positional Audio") == 0) {
                defaultHandle = handle;
            }
            if (wantedHandle == NULL && defaultHandle == NULL) {
                wantedHandle = handle;
            }
        } while (AIL_enumerate_3D_providers(&providerIter, &handle, &name));
    }

    milesGlob.provider_3D = NULL;

    if (wantedHandle != NULL) {
        if (AIL_open_3D_provider(wantedHandle) != 0) {
            Com_Printf((const char *)"couldn't open 3D provider '%s': %s\n", wantedName, AIL_last_error());

        } else {
            milesGlob.provider_3D = wantedHandle;
        }
    }

    if (milesGlob.provider_3D != NULL) {
        goto configure_provider;
    }

    if (defaultHandle == NULL || defaultHandle == wantedHandle) {
        goto shutdown_and_fail;
    }

    if (wantedName[0] != 0) {
        if (stricmp(wantedName, (const char *)"Miles Fast 2D Positional Audio") != 0) {
            Com_Printf((const char *)"trying to use 'Miles Fast 2D Positional Audio' instead of '%s'\n", wantedName);
        }
    }

    if (AIL_open_3D_provider(defaultHandle) != 0) {
        Com_Printf((const char *)"couldn't open 3D provider '%s': %s\n", (const char *)"Miles Fast 2D Positional Audio", AIL_last_error());
        if (milesGlob.provider_3D == NULL) {
            goto shutdown_and_fail;
        }
    } else {
        milesGlob.provider_3D = defaultHandle;
        Dvar_SetString(mss_3d_provider, (const char *)"Miles Fast 2D Positional Audio");
        if (milesGlob.provider_3D == NULL) {
            goto shutdown_and_fail;
        }
    }

configure_provider:

    sndGlob = &g_snd;
    sndGlob->Initialized3d = 1;
    AIL_3D_provider_attribute(milesGlob.provider_3D, (const char *)"Maximum supported samples", (void *)&sndGlob->max_3D_channels);
    if (sndGlob->max_3D_channels > 0x20) {
        sndGlob->max_3D_channels = 0x20;
    }
    Com_Printf((const char *)"%i max 3D channels\n", sndGlob->max_3D_channels);
    {
        int distFactor = 0x3cd013a9;
        AIL_set_3D_distance_factor(milesGlob.provider_3D, *(float *)&distFactor);
    }
    AIL_set_3D_rolloff_factor(milesGlob.provider_3D, 0.0f);

    sndGlob = &g_snd;
    for (i = 0; i < sndGlob->max_2D_channels; i++) {
        handle_2D_004a3ad4[i] = AIL_allocate_sample_handle(milesGlob.driver_2D);
        if (handle_2D_004a3ad4[i] == NULL) {
            Com_Error(1, (const char *)"\x15MILES 2D sound sample allocation failed on channel %i\n", i + 1);
        }
    }

    sndGlob = &g_snd;
    for (i = 0; i < sndGlob->max_3D_channels; i++) {
        milesGlob.handle_3D[i] = AIL_allocate_3D_sample_handle(milesGlob.provider_3D);
        if (milesGlob.handle_3D[i] == NULL) {
            Com_Error(1, (const char *)"\x15MILES 3D sound sample allocation failed on channel %i\n", i + 1);
        }
    }

    sndGlob = &g_snd;
    sndGlob->ambient_track = 2;
    return 1;

shutdown_and_fail:
    AIL_shutdown();
    memset(&milesGlob, 0, sizeof(MssLocal));

startup_failed:
    if (Dvar_GetInt((const char *)"r_vc_compile") == 2) {
        return 0;
    }
    Com_Printf((const char *)"Miles sound system initialization failed\n");
    return 0;
}

void SND_SetStreamChannelFromSaveInfo(int index, snd_save_stream_t *info)
{
    float pan = info->pan;
    float vol = info->volume * g_snd.volume;
    snd_local_t *sndGlob = &g_snd;
    if (sndGlob->chaninfo[index].srcChannelCount == 2) {
        const snd_alias_t *pA = sndGlob->chaninfo[index].pAlias0;
        int ch = (pA->flags & 0x780) >> 7;
        if (!SND_IsAliasChannel3D(ch)) {
            AIL_set_stream_volume_levels(milesGlob.handle_stream[index - 0x20], vol, vol);
            return;
        }
    }
    {
        float leftVol = (1.0f - pan) * vol;
        float rightVol = pan * vol;
        AIL_set_stream_volume_levels(milesGlob.handle_stream[index - 0x20], leftVol, rightVol);
    }
}

void SND_DriverPostUpdate(int frametime)
{
    void *rawHandle;
    float vol;
    int bufNum;

    rawHandle = milesGlob.raw.handle;
    if (rawHandle == NULL) {
        return;
    }

    vol = 0.5f * g_snd.volume;
    AIL_set_sample_volume_levels(rawHandle, vol, vol);

    if (milesGlob.raw.bufReady[milesGlob.raw.readBuf] == 0) {
        return;
    }

    bufNum = AIL_sample_buffer_ready(milesGlob.raw.handle);
    if (bufNum == -1) {
        return;
    }

    milesGlob.raw.sampleTime += (double)milesGlob.raw.bufSize * milesGlob.raw.bufRate;

    AIL_load_sample_buffer(milesGlob.raw.handle, bufNum,
                           (void *)(milesGlob.raw.buf + milesGlob.raw.readBuf * milesGlob.raw.bufSize),
                           milesGlob.raw.bufSize);

    milesGlob.raw.bufReady[milesGlob.raw.readBuf] = 0;
    milesGlob.raw.readBuf = (milesGlob.raw.readBuf + 1) % 32;
}
