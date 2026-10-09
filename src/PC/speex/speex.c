/* ASM dump from: speex.c */
/* Original path: PC/speex/speex.c */

#include "common_types.h"
#include "imports.h"

float * speex_encoder_init(const SpeexMode *mode);
float * speex_decoder_init(const SpeexMode *mode);
void speex_encoder_destroy(float *state);
void speex_decoder_destroy(float *state);
int speex_encode_native(float *state, spx_word16_t *in, SpeexBits *bits);
int speex_decode_native(float *state, SpeexBits *bits, spx_word16_t *out);
int speex_encoder_ctl(float *state, int request, float *ptr);
int speex_decode(float *state, SpeexBits *bits, float *out);
int speex_decoder_ctl(float *state, int request, float *ptr);
int nb_mode_query(const float *mode, int request, float *ptr);
int wb_mode_query(const float *mode, int request, float *ptr);
int speex_encode_int(float *state, const short *in, SpeexBits *bits);
extern void speex_warning_int(const char *msg, int val);

/* line 50 */
float * speex_encoder_init(const SpeexMode *mode)
{
    return ((float *(*)(const SpeexMode *))mode->enc_init)(mode);
}

/* line 55 */
float * speex_decoder_init(const SpeexMode *mode)
{
    return ((float *(*)(const SpeexMode *))mode->dec_init)(mode);
}

/* line 60 */
void speex_encoder_destroy(float *state)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    ((void (*)(float *))mode->enc_destroy)(state);
}

/* line 65 */
void speex_decoder_destroy(float *state)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    ((void (*)(float *))mode->dec_destroy)(state);
}

/* line 72 */
int speex_encode_native(float *state, spx_word16_t *in, SpeexBits *bits)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    return ((int (*)(float *, spx_word16_t *, SpeexBits *))mode->enc)(state, in, bits);
}

/* line 77 */
int speex_decode_native(float *state, SpeexBits *bits, spx_word16_t *out)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    return ((int (*)(float *, SpeexBits *, spx_word16_t *))mode->dec)(state, bits, out);
}

/* line 175 */
int speex_encoder_ctl(float *state, int request, float *ptr)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    return ((int (*)(float *, int, float *))mode->enc_ctl)(state, request, ptr);
}

/* line 147 */
int speex_decode(float *state, SpeexBits *bits, float *out)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    return ((int (*)(float *, SpeexBits *, float *))mode->dec)(state, bits, out);
}

/* line 180 */
int speex_decoder_ctl(float *state, int request, float *ptr)
{
    const SpeexMode *mode = *(const SpeexMode **)state;
    return ((int (*)(float *, int, float *))mode->dec_ctl)(state, request, ptr);
}

/* line 187 */
int nb_mode_query(const float *mode, int request, float *ptr)
{
    switch (request) {
    case 0:
        *(int *)ptr = *(int *)mode;
        return 0;
    case 1:
        if (*(int *)ptr == 0) {
            *(int *)ptr = 5;
            return 0;
        }
        {
            const SpeexSubmode *submodePtr = ((const SpeexNBMode *)mode)->submodes[*(int *)ptr];
            if (submodePtr) {
                *(int *)ptr = submodePtr->bits_per_frame;
            } else {
                *(int *)ptr = -1;
            }
        }
        return 0;
    default:
        speex_warning_int("Unknown nb_mode_query request: ", request);
        return -1;
    }
}

/* line 211 */
int wb_mode_query(const float *mode, int request, float *ptr)
{
    switch (request) {
    case 0:
        *(int *)ptr = ((const SpeexSBMode *)mode)->frameSize * 2;
        return 0;
    case 1:
        if (*(int *)ptr == 0) {
            *(int *)ptr = 4;
            return 0;
        }
        {
            const SpeexSubmode *submodePtr = ((const SpeexSBMode *)mode)->submodes[*(int *)ptr];
            if (submodePtr) {
                *(int *)ptr = submodePtr->bits_per_frame;
            } else {
                *(int *)ptr = -1;
            }
        }
        return 0;
    default:
        speex_warning_int("Unknown wb_mode_query request: ", request);
        return -1;
    }
}

/* line 136 */
int speex_encode_int(float *state, const short *in, SpeexBits *bits)
{
    float float_in[640];
    int N;
    int i;

    speex_encoder_ctl(state, 3, (float *)&N);

    for (i = 0; i < N; i++) {
        float_in[i] = (float)in[i];
    }

    return speex_encode_native(state, (spx_word16_t *)float_in, bits);
}
