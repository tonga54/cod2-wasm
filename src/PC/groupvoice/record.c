
#include "common_types.h"
#include "imports.h"
#include <string.h>


int samples_in_partial_audio_buffer = 0x0;

extern char enc_buffer[4096];
extern int enc_buffer_pos;
extern short int partial_audio_buffer[640];
extern int samples_in_partial_audio_buffer;

extern int DSOUNDRecord_Start(recordingSample_t *sample);
extern int DSOUNDRecord_Stop(recordingSample_t *sample);
extern recordingSample_t *DSOUNDRecord_NewSample(void);
extern int DSOUNDRecord_DestroySample(recordingSample_t *sample);
extern void DSOUNDRecord_Shutdown(void);
extern void DSOUNDRecord_Frame(void);
extern int DSOUNDRecord_Init(void);
extern Bool Encode_Shutdown(void);
extern int Encode_GetFrameSize(void);
extern int Encode_Sample(void *audioData, void *outBuf, int outSize);
extern int Voice_SendVoiceData(void);
extern int IsTalking(void);
extern void CL_VoiceTransmit(void);
extern int Client_SendVoiceData(int size, void *data);
extern void Com_Printf(const char *msg, ...);

extern byte *encode_vol_ptr;
extern byte *voice_scale_ptr;
extern byte *record_callback_ptr;

int Record_Start(recordingSample_t *sample);
int Record_Stop(recordingSample_t *sample);
recordingSample_t * Record_NewSample(void);
int Record_DestroySample(recordingSample_t *sample);
void Record_Shutdown(void);
void Record_Frame(void);
int Record_QueueAudioDataForEncoding(audioSample_t *sample);
int Record_AudioCallback(audioSample_t *sample);
int Record_Init(int callInit, const void *handle);

int Record_Start(recordingSample_t *sample)
{
    return DSOUNDRecord_Start(sample);
}

int Record_Stop(recordingSample_t *sample)
{
    return DSOUNDRecord_Stop(sample);
}

recordingSample_t *Record_NewSample(void)
{
    return DSOUNDRecord_NewSample();
}

int Record_DestroySample(recordingSample_t *sample)
{
    return DSOUNDRecord_DestroySample(sample);
}

void Record_Shutdown(void)
{
    Encode_Shutdown();
    DSOUNDRecord_Shutdown();
}

void Record_Frame(void)
{
    DSOUNDRecord_Frame();
}

int Record_QueueAudioDataForEncoding(audioSample_t *sample)
{
    short audio_buffer[4096];
    int total_bytes;
    int encode_frame_size;
    int ii;
    float *p_vol;
    short *data;
    int numSamples;
    int channels;
    int partial;

    p_vol = *(float **)&encode_vol_ptr;
    *p_vol = 0.0f;

    data = (short *)sample->buffer;
    numSamples = sample->lengthInSamples;
    channels = sample->bytesPerSample;

    if (channels == 2 && numSamples > 0) {
        float scale = **(float **)&voice_scale_ptr;
        for (ii = 0; ii < numSamples; ii++) {
            float fval = (float)data[ii] * scale;
            data[ii] = (short)(int)fval;
            int abs_val = (short)(int)fval;
            if (abs_val < 0) abs_val = -abs_val;
            *p_vol += (float)abs_val;
        }
        *p_vol /= (float)numSamples;
    }

    if (!Voice_SendVoiceData()) {
        return 0;
    }

    if (!IsTalking()) {
        CL_VoiceTransmit();
        return 0;
    }

    encode_frame_size = Encode_GetFrameSize();
    sample->sampleOffset = 0;

    if (encode_frame_size <= 0) {
        Com_Printf("Invalid encode frame size of %i\n", encode_frame_size);
        return 0;
    }

    if (sample->lengthInBytes <= 0) {
        Com_Printf("Invalid sample length of %i samples\n", numSamples);
        return 0;
    }

    total_bytes = 0;

    while (1) {
        int samplesProcessed = sample->sampleOffset;
        partial = samples_in_partial_audio_buffer;

        if (numSamples - samplesProcessed + partial < encode_frame_size) {

            if (samplesProcessed < numSamples) {
                int remaining = numSamples - samplesProcessed;
                memcpy(partial_audio_buffer + partial,
                       data + samplesProcessed * channels,
                       remaining * channels);
                samples_in_partial_audio_buffer += numSamples - sample->sampleOffset;
            }
            return total_bytes;
        }

        int partial_take = 0;
        if (partial > 0) {
            int max_take = encode_frame_size - 4;
            if (partial < max_take)
                partial_take = partial;
            else
                partial_take = max_take;
            samples_in_partial_audio_buffer = partial_take;
            memcpy(audio_buffer, partial_audio_buffer, partial_take * channels);
            samples_in_partial_audio_buffer -= partial_take;
            partial = partial_take;
        }

        {
            int src_take = encode_frame_size - partial;
            samplesProcessed = sample->sampleOffset;
            memcpy((byte *)audio_buffer + partial * 2,
                   data + samplesProcessed * channels,
                   src_take * channels);
            sample->sampleOffset += src_take;
        }

        {
            int avail = 4096 - enc_buffer_pos;
            int encoded = Encode_Sample(audio_buffer, enc_buffer + enc_buffer_pos, avail);
            total_bytes += encoded;
            Client_SendVoiceData(encoded, enc_buffer + enc_buffer_pos);
        }
    }
}

int Record_AudioCallback(audioSample_t *sample)
{
    return Record_QueueAudioDataForEncoding(sample);
}

int Record_Init(int callInit, const void *handle)
{
    *(void **)record_callback_ptr = (void *)Record_AudioCallback;
    return DSOUNDRecord_Init();
}
