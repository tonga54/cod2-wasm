
#include "common_types.h"
#include "imports.h"


int g_current_bandwidth_setting = 0x0;
int g_frame_size = 0x0;
int g_speex_initialized = 0x0;

extern int speex_encoder_ctl(void *state, int request, void *ptr);
extern void *speex_encoder_init(const void *mode);
extern void speex_encoder_destroy(void *state);
extern void speex_bits_init(SpeexBits *bits);
extern void speex_bits_destroy(SpeexBits *bits);
extern void speex_bits_reset(SpeexBits *bits);
extern int speex_encode_int(void *state, short *in, SpeexBits *bits);
extern int speex_bits_write(SpeexBits *bits, char *bytes, int max_len);
extern void Com_Printf(const char *fmt, ...);

extern const void *speex_nb_mode_ptr;
extern const void *speex_wb_mode_ptr;
extern const void *speex_uwb_mode_ptr;

extern int g_encoder_quality;
extern int g_encoder_samplerate;
extern int g_current_bandwidth_setting;
extern int g_frame_size;
extern int g_speex_initialized;
static void *g_encoder;
static struct SpeexBits encodeBits;

extern void *speex_quality_ptr;

void Encode_SetOptions(int frequency, int quality)
{
    char yes = 1;

    speex_encoder_ctl(g_encoder, 0x18, &frequency);
    speex_encoder_ctl(g_encoder, 4, &quality);
    speex_encoder_ctl(g_encoder, 3, &g_frame_size);
    speex_encoder_ctl(g_encoder, 0x1e, &yes);
    speex_encoder_ctl(g_encoder, 0x22, &yes);

    g_encoder_quality = quality;
    g_encoder_samplerate = frequency;
}

Bool Encode_Init(int bandwidthEnum)
{
    const void *mode;

    switch (bandwidthEnum) {
    case 0:
        mode = speex_nb_mode_ptr;
        break;
    case 1:
        mode = speex_wb_mode_ptr;
        break;
    case 2:
        mode = speex_uwb_mode_ptr;
        break;
    default:
        Com_Printf("Unknown bandwidth mode %i\n", bandwidthEnum);
        return 0;
    }

    g_encoder = speex_encoder_init(mode);
    speex_bits_init(&encodeBits);
    g_speex_initialized = 1;

    Encode_SetOptions(g_encoder_samplerate, g_encoder_quality);
    speex_encoder_ctl(g_encoder, 3, &g_frame_size);

    g_current_bandwidth_setting = bandwidthEnum;

    return g_encoder != 0;
}

Bool Encode_Shutdown(void)
{
    if (g_encoder) {
        speex_encoder_destroy(g_encoder);
    }
    g_encoder = 0;

    if (g_speex_initialized) {
        speex_bits_destroy(&encodeBits);
        g_speex_initialized = 0;
    }

    return 1;
}

int Encode_Sample(short *buffer_in, char *buffer_out, int maxLength)
{
    int quality;

    quality = *(int *)((char *)*(void **)speex_quality_ptr + 8);
    if (quality != g_encoder_quality) {
        g_encoder_quality = quality;
        speex_encoder_ctl(g_encoder, 4, &g_encoder_quality);
    }

    speex_bits_reset(&encodeBits);
    speex_encode_int(g_encoder, buffer_in, &encodeBits);

    return speex_bits_write(&encodeBits, buffer_out, maxLength);
}

int Encode_GetFrameSize(void)
{
    return g_frame_size;
}
