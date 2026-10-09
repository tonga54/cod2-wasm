#include "common_types.h"
#include "imports.h"

void StMutexLock_StMutexLock(char *lock, void *mutex);
void ZN11StMutexLockD1Ev(char *lock);
void ZN6CMutexD1Ev(void *mutex);
void CMutex_CMutex(void *mutex);
void __ZdlPv(void *ptr);
float CSoundEngine_GetDefaultMaxDistance(const CSoundEngine *inEngine);
float CSoundEngine_GetDefaultReferenceDistance(const CSoundEngine *inEngine);
void free(void *ptr);

#define CSOUNDOBJECT_VTABLE_PTR ((void *)0x332548)

#if defined(__EMSCRIPTEN__) || (defined(COD2_NATIVE_PORTABLE_DATA) && !defined(COD2_X64))
AIL_file_open_callback __ZN12CSoundObject13sOpenCallbackE;
AIL_file_close_callback __ZN12CSoundObject14sCloseCallbackE;
AIL_file_seek_callback __ZN12CSoundObject13sSeekCallbackE;
AIL_file_read_callback __ZN12CSoundObject13sReadCallbackE;
#else
extern AIL_file_open_callback __ZN12CSoundObject13sOpenCallbackE;
extern AIL_file_close_callback __ZN12CSoundObject14sCloseCallbackE;
extern AIL_file_seek_callback __ZN12CSoundObject13sSeekCallbackE;
extern AIL_file_read_callback __ZN12CSoundObject13sReadCallbackE;
#endif

void ZN12CSoundObjectD2Ev(char *_this);
void ZN12CSoundObjectD1Ev(char *_this);
void ZN12CSoundObjectD0Ev(char *_this);
void CSoundObject_Release(char *_this);
Boolean CSoundObject_IsAvailable(char *_this);
UInt32 CSoundObject_GetSoundType(char *_this);
Boolean CSoundObject_Is3DSound(char *_this);
void CSoundObject_TheadIdle(char *_this);
void CSoundObject_set_sample_address(char *_this, const void *start, long unsigned int len);
void CSoundObject_set_sample_type(const CSoundObject *_this, long int format, long unsigned int flags);
void CSoundObject_set_sample_volume(const CSoundObject *_this, float volume);
void CSoundObject_stop_sample(char *_this);
void CSoundObject_resume_sample(char *_this);
void CSoundObject_end_sample(char *_this);
void CSoundObject_set_sample_adpcm_block_size(const CSoundObject *_this, long unsigned int blocksize);
float CSoundObject_get_sample_volume(char *_this);
void CSoundObject_get_sample_volume_levels(char *_this, float *left_level, float *right_level);
void CSoundObject_set_sample_volume_levels(const CSoundObject *_this, float left_level, float right_level);
void CSoundObject_get_sample_volume_pan(char *_this, float *volume, float *pan);
void CSoundObject_set_sample_reverb_levels(char *_this, float dry_level, float wet_level);
long unsigned int CSoundObject_get_sample_position(char *_this);
void CSoundObject_set_sample_position(char *_this, long unsigned int position);
long int CSoundObject_get_sample_playback_rate(char *_this);
void CSoundObject_set_sample_playback_rate(const CSoundObject *_this, long int playback_rate);
void CSoundObject_set_sample_loop_count(char *_this, long unsigned int loops);
long unsigned int CSoundObject_get_sample_status(char *_this);
long unsigned int CSoundObject_get_sample_length(char *_this);
long int CSoundObject_sample_buffer_ready(const CSoundObject *_this);
void CSoundObject_load_sample_buffer(const CSoundObject *_this, long unsigned int buff_num, const void *buffer, long unsigned int len);
void CSoundObject_set_3D_position(const CSoundObject *_this, float X, float Y, float Z);
void CSoundObject_get_3D_position(char *_this, float *X, float *Y, float *Z);
void CSoundObject_set_3D_sample_distances(const CSoundObject *_this, float max_dist, float min_dist);
void CSoundObject_set_3D_sample_effects_level(char *_this, float effects_level);
bool CSoundObject_open_stream(char *_this, const char *filename);
void CSoundObject_close_stream(char *_this);
void CSoundObject_get_stream_info(char *_this, long int *datarate, long int *sndtype, long int *length, long int *memory);
void CSoundObject_set_file_callbacks(AIL_file_open_callback opencb, AIL_file_close_callback closecb, AIL_file_seek_callback seekcb, AIL_file_read_callback readcb);
void CSoundObject_FormatToDescription(long int mssPlaybackRate, long int mssFormat, AudioStreamBasicDescription *outDescription);
bool CSoundObject_IsEqualDescriptions(const AudioStreamBasicDescription *inDesc1, const AudioStreamBasicDescription *inDesc2);
Boolean CSoundObject_SampleBuffersEmpty(char *_this);
void CSoundObject_ChangedVolume(char *_this);
void CSoundObject_ChangedFormat(char *_this);
void CSoundObject_Changed3DPosition(char *_this);
void CSoundObject_Changed3DDistances(char *_this);
long unsigned int CSoundObject_file_open_callback(const char *Filename, long unsigned int *FileHandle);
void CSoundObject_file_close_callback(long unsigned int FileHandle);
long int CSoundObject_file_seek_callback(long unsigned int FileHandle, long int Offset, long unsigned int Type);
long unsigned int CSoundObject_file_read_callback(long unsigned int FileHandle, void *Buffer, long unsigned int Bytes);
void CSoundObject_CSoundObject(char *_this, CSoundEngine *inEngine, UInt32 inBusIndex);
void CSoundObject_get_sample_ms_position(const CSoundObject *_this, long int *total_milliseconds, long int *current_milliseconds);
void CSoundObject_set_sample_ms_position(char *_this, long int milliseconds);
void CSoundObject_init_sample(const CSoundObject *_this);
long int CSoundObject_set_sample_info(const CSoundObject *_this, const AILSOUNDINFO *info);

void ZN12CSoundObjectD2Ev(char *_this)
{
    *(int *)(_this) = (int)CSOUNDOBJECT_VTABLE_PTR;
    ZN6CMutexD1Ev(_this + 0x20);
}

void ZN12CSoundObjectD1Ev(char *_this)
{
    *(int *)(_this) = (int)CSOUNDOBJECT_VTABLE_PTR;
    ZN6CMutexD1Ev(_this + 0x20);
}

void ZN12CSoundObjectD0Ev(char *_this)
{
    *(int *)(_this) = (int)CSOUNDOBJECT_VTABLE_PTR;
    ZN6CMutexD1Ev(_this + 0x20);
    __ZdlPv(_this);
}

void CSoundObject_Release(char *_this)
{
    int (**vtable)(char *);
    int i;
    char *p;

    vtable = *(int (***)(char *))_this;
    ((void (*)(char *))vtable[3])(_this);

    vtable = *(int (***)(char *))_this;
    ((void (*)(char *))vtable[11])(_this);

    p = _this;
    for (i = 0; i < 4; i++) {
        void *buf = *(void **)(p + 0x4c);
        if (buf != 0) {
            free(buf);
            *(void **)(p + 0x4c) = 0;
        }
        p += 4;
    }

    *(int *)(_this + 0x5c) = 0;
    *(unsigned char *)(_this + 0xc) = 1;
}

Boolean CSoundObject_IsAvailable(char *_this)
{
    return *(unsigned char *)(_this + 0xc);
}

UInt32 CSoundObject_GetSoundType(char *_this)
{
    return *(int *)(_this + 0x10);
}

Boolean CSoundObject_Is3DSound(char *_this)
{
    return *(int *)(_this + 0x10) == 1;
}

void CSoundObject_TheadIdle(char *_this)
{
    (void)_this;
}

void CSoundObject_set_sample_address(char *_this, const void *start, unsigned long len)
{
    char lock[8];
    char *p;
    int i;

    StMutexLock_StMutexLock(lock, _this + 0x20);

    *(const void **)(_this + 0x84) = start;
    *(unsigned long *)(_this + 0x94) = len;

    p = _this;
    for (i = 0; i < 3; i++) {
        *(int *)(p + 0x88) = 0;
        *(int *)(p + 0x98) = 0;
        p += 4;
    }

    *(int *)(_this + 0xac) = 0;
    *(unsigned long *)(_this + 0xb0) = len;
    *(unsigned char *)(_this + 0x14) = 0;

    ZN11StMutexLockD1Ev(lock);
}

static inline __attribute__((always_inline)) void CSoundObject_CallVirtual(char *obj, int offset)
{
    ((void (*)(char *))(*(void ***)obj)[offset / 4])(obj);
}

static inline __attribute__((always_inline)) void CSoundObject_CallVirtualUInt(char *obj, int offset, unsigned int value)
{
    ((void (*)(char *, unsigned int))(*(void ***)obj)[offset / 4])(obj, value);
}

static inline __attribute__((always_inline)) void CSoundObject_Lock(char *lock, char *obj)
{
    StMutexLock_StMutexLock(lock, obj + 0x20);
}

static inline __attribute__((always_inline)) void CSoundObject_Unlock(char *lock)
{
    ZN11StMutexLockD1Ev(lock);
}

void CSoundObject_set_sample_type(const CSoundObject *_this, long int format, long unsigned int flags)
{
    char *obj = (char *)_this;
    char lock[8];
    int channels;

    (void)flags;
    CSoundObject_Lock(lock, obj);

    if (*(long int *)(obj + 0xa8) != format) {
        *(long int *)(obj + 0xa8) = format;

        switch (format) {
        case 1:
        case 2:
            channels = 2;
            break;
        case 3:
        case 8:
            channels = 4;
            break;
        case 10:
            channels = 8;
            break;
        default:
            channels = 1;
            break;
        }

        *(int *)(obj + 0x1c) = channels;
        CSoundObject_CallVirtual(obj, 0x34);
    }

    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_adpcm_block_size(const CSoundObject *_this, long unsigned int blocksize)
{
    char *obj = (char *)_this;
    char lock[8];

    CSoundObject_Lock(lock, obj);
    if (*(long unsigned int *)(obj + 0xa4) != blocksize) {
        *(long unsigned int *)(obj + 0xa4) = blocksize;
        CSoundObject_CallVirtual(obj, 0x34);
    }
    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_volume(const CSoundObject *_this, float volume)
{
    char *obj = (char *)_this;
    char lock[8];

    CSoundObject_Lock(lock, obj);
    if (*(float *)(obj + 0x64) != volume) {
        *(float *)(obj + 0x64) = volume;
        CSoundObject_CallVirtual(obj, 0x30);
    }
    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_volume_levels(const CSoundObject *_this, float left_level, float right_level)
{
    char *obj = (char *)_this;
    char lock[8];

    CSoundObject_Lock(lock, obj);

    if (!(*(int *)(obj + 0xa8) & 2)) {
        left_level += left_level;
        right_level += right_level;
    }

    if (left_level < 0.0f)
        left_level = 0.0f;
    else if (left_level > 1.0f)
        left_level = 1.0f;

    if (right_level < 0.0f)
        right_level = 0.0f;
    else if (right_level > 1.0f)
        right_level = 1.0f;

    if (*(float *)(obj + 0x68) != left_level || *(float *)(obj + 0x6c) != right_level) {
        *(float *)(obj + 0x68) = left_level;
        *(float *)(obj + 0x6c) = right_level;
        CSoundObject_CallVirtual(obj, 0x30);
    }

    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_reverb_levels(char *_this, float dry_level, float wet_level)
{
    char lock[8];

    CSoundObject_Lock(lock, _this);
    *(float *)(_this + 0x74) = dry_level;
    *(float *)(_this + 0x78) = wet_level;
    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_position(char *_this, long unsigned int position)
{
    char lock[8];
    unsigned int format;

    CSoundObject_Lock(lock, _this);

    *(long unsigned int *)(_this + 0xac) = position;
    format = *(unsigned int *)(_this + 0xa8);
    if (format & 1) {
        *(long unsigned int *)(_this + 0xac) = position & ~1ul;
    } else if (format & 8) {
        *(long unsigned int *)(_this + 0xac) = (position + 3) & ~3ul;
    } else {
        *(long unsigned int *)(_this + 0xac) = position;
    }

    CSoundObject_Unlock(lock);
}

long int CSoundObject_get_sample_playback_rate(char *_this)
{
    return *(long int *)(_this + 0x7c);
}

void CSoundObject_set_sample_playback_rate(const CSoundObject *_this, long int playback_rate)
{
    char *obj = (char *)_this;
    char lock[8];

    CSoundObject_Lock(lock, obj);
    if (*(long int *)(obj + 0x7c) != playback_rate) {
        *(long int *)(obj + 0x7c) = playback_rate;
        CSoundObject_CallVirtual(obj, 0x34);
    }
    CSoundObject_Unlock(lock);
}

void CSoundObject_set_sample_loop_count(char *_this, long unsigned int loops)
{
    char lock[8];

    CSoundObject_Lock(lock, _this);
    *(long unsigned int *)(_this + 0x80) = loops;
    CSoundObject_Unlock(lock);
}

long unsigned int CSoundObject_get_sample_status(char *_this)
{
    char lock[8];
    long unsigned int status;

    CSoundObject_Lock(lock, _this);
    status = *(long unsigned int *)(_this + 0x60);
    CSoundObject_Unlock(lock);

    return status;
}

long int CSoundObject_sample_buffer_ready(const CSoundObject *_this)
{
    char *obj = (char *)_this;
    char lock[8];
    int i;

    CSoundObject_Lock(lock, obj);
    obj[0x14] = 1;

    i = *(int *)(obj + 0x18);
    if (i <= 3) {
        while (i < 4 && *(void **)(obj + 0x84 + i * 4) != 0) {
            ++i;
        }
        if (i != 4) {
            CSoundObject_Unlock(lock);
            return i;
        }
    }

    if (i == 0) {
        CSoundObject_Unlock(lock);
        return -1;
    }

    if (*(void **)(obj + 0x84) == 0) {
        CSoundObject_Unlock(lock);
        return 0;
    }

    {
        int slot;
        for (slot = 1; slot < i; ++slot) {
            if (*(void **)(obj + 0x84 + slot * 4) == 0) {
                CSoundObject_Unlock(lock);
                return slot;
            }
        }
    }

    CSoundObject_Unlock(lock);
    return -1;
}

void CSoundObject_load_sample_buffer(const CSoundObject *_this, long unsigned int buff_num, const void *buffer, long unsigned int len)
{
    char *obj = (char *)_this;

    obj[0x14] = 1;
    if (buff_num <= 3) {
        char lock[8];

        CSoundObject_Lock(lock, obj);
        if (*(void **)(obj + 0x84 + buff_num * 4) == 0) {
            *(const void **)(obj + 0x84 + buff_num * 4) = buffer;
            *(long unsigned int *)(obj + 0x94 + buff_num * 4) = len;
        }
        if (*(long unsigned int *)(obj + 0xb0) == 0) {
            *(long unsigned int *)(obj + 0xb0) = len;
        }
        CSoundObject_Unlock(lock);
    }

    CSoundObject_CallVirtual(obj, 0x10);
}

void CSoundObject_set_3D_position(const CSoundObject *_this, float X, float Y, float Z)
{
    char *obj = (char *)_this;
    char lock[8];

    CSoundObject_Lock(lock, obj);
    *(float *)(obj + 0xb4) = X;
    *(float *)(obj + 0xb8) = Y;
    *(float *)(obj + 0xbc) = -Z;
    CSoundObject_CallVirtual(obj, 0x38);
    CSoundObject_Unlock(lock);
}

void CSoundObject_set_3D_sample_effects_level(char *_this, float effects_level)
{
    char lock[8];

    CSoundObject_Lock(lock, _this);
    *(float *)(_this + 0x78) = effects_level;
    CSoundObject_Unlock(lock);
}

void CSoundObject_get_stream_info(char *_this, long int *datarate, long int *sndtype, long int *length, long int *memory)
{
    if (datarate)
        *datarate = 0;
    if (sndtype)
        *sndtype = *(long int *)(_this + 0xa8);
    if (length)
        *length = 0;
    if (memory)
        *memory = 0;
}

void CSoundObject_set_file_callbacks(AIL_file_open_callback opencb, AIL_file_close_callback closecb, AIL_file_seek_callback seekcb, AIL_file_read_callback readcb)
{
    __ZN12CSoundObject13sOpenCallbackE = opencb;
    __ZN12CSoundObject14sCloseCallbackE = closecb;
    __ZN12CSoundObject13sSeekCallbackE = seekcb;
    __ZN12CSoundObject13sReadCallbackE = readcb;
}

void CSoundObject_init_sample(const CSoundObject *_this)
{
    char *obj = (char *)_this;
    char outerLock[16];
    char lock[8];

    CSoundObject_Lock(outerLock, obj);

    CSoundObject_CallVirtual(obj, 0x0c);
    CSoundObject_CallVirtual(obj, 0x2c);
    *(int *)(obj + 0x60) = 2;

    CSoundObject_Lock(lock, obj);
    *(void **)(obj + 0x84) = 0;
    *(long unsigned int *)(obj + 0x94) = 0;
    {
        int i;
        for (i = 0; i < 3; ++i) {
            *(void **)(obj + 0x88 + i * 4) = 0;
            *(long unsigned int *)(obj + 0x98 + i * 4) = 0;
        }
    }
    *(long unsigned int *)(obj + 0xac) = 0;
    *(long unsigned int *)(obj + 0xb0) = 0;
    obj[0x14] = 0;
    CSoundObject_Unlock(lock);

    *(long unsigned int *)(obj + 0x80) = 1;
    CSoundObject_CallVirtualUInt(obj, 0x20, 0x2b11);

    CSoundObject_Lock(lock, obj);
    if (*(long int *)(obj + 0xa8) != 0) {
        *(long int *)(obj + 0xa8) = 0;
        *(int *)(obj + 0x1c) = 1;
        CSoundObject_CallVirtual(obj, 0x34);
    }
    CSoundObject_Unlock(lock);

    CSoundObject_Lock(lock, obj);
    if (*(float *)(obj + 0x64) != 1.0f) {
        *(float *)(obj + 0x64) = 1.0f;
        CSoundObject_CallVirtual(obj, 0x30);
    }
    CSoundObject_Unlock(lock);

    *(float *)(obj + 0x70) = 0.5f;
    *(long unsigned int *)(obj + 0xac) = 0;
    *(long unsigned int *)(obj + 0xb0) = 0;
    CSoundObject_CallVirtual(obj, 0x34);

    if (*(long int *)(obj + 0x10) == 1) {
        CSoundObject_CallVirtual(obj, 0x38);
    }

    obj[0x14] = 0;
    CSoundObject_Unlock(outerLock);
}
