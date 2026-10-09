#include "audio_stubs.h"

OSStatus NewAUGraph(AUGraph *outGraph)
{
    if (outGraph)
        *outGraph = 0;
    return 0;
}

OSStatus DisposeAUGraph(AUGraph graph)
{
    return 0;
}

OSStatus AUGraphInitialize(AUGraph graph)
{
    return 0;
}

OSStatus AUGraphStart(AUGraph graph)
{
    return 0;
}

OSStatus AUGraphStop(AUGraph graph)
{
    return 0;
}

OSStatus AUGraphAddNode(AUGraph graph, const ComponentDescription *desc, AUNode *outNode)
{
    if (outNode)
        *outNode = 0;
    return 0;
}

OSStatus AUGraphConnectNodeInput(AUGraph graph, AUNode srcNode, unsigned int srcOutput, AUNode destNode, unsigned int destInput)
{
    return 0;
}

OSStatus AUGraphNodeInfo(AUGraph graph, AUNode node, ComponentDescription *outDesc, AudioUnit *outUnit)
{
    if (outUnit)
        *outUnit = 0;
    return 0;
}

OSStatus AUGraphOpen(AUGraph graph)
{
    return 0;
}

OSStatus AudioUnitSetProperty(AudioUnit unit, AudioUnitPropertyID id, AudioUnitScope scope, AudioUnitElement elem, const void *data, UInt32 size)
{
    return 0;
}

OSStatus AudioUnitGetProperty(AudioUnit unit, AudioUnitPropertyID id, AudioUnitScope scope, AudioUnitElement elem, void *data, UInt32 *size)
{
    return 0;
}

OSStatus AudioUnitSetParameter(AudioUnit unit, unsigned int paramID, unsigned int scope, unsigned int elem, float value, unsigned int offset)
{
    return 0;
}

OSStatus AudioUnitRender(AudioUnit unit, unsigned int *ioActionFlags, const void *inTimeStamp, unsigned int inOutputBusNumber, unsigned int inNumberFrames, void *ioData)
{
    return 0;
}

OSStatus AudioOutputUnitStart(AudioUnit unit)
{
    return 0;
}

OSStatus AudioOutputUnitStop(AudioUnit unit)
{
    return 0;
}

OSStatus AudioDeviceGetProperty(AudioDeviceID dev, unsigned int channel, int isInput, unsigned int propID, unsigned int *size, void *data)
{
    return 0;
}

OSStatus AudioDeviceSetProperty(AudioDeviceID dev, const void *when, unsigned int channel, int isInput, unsigned int propID, unsigned int size, const void *data)
{
    return 0;
}

OSStatus AudioHardwareGetProperty(unsigned int propID, unsigned int *size, void *data)
{
    return 0;
}

#ifndef __EMSCRIPTEN__
#endif

int CAudioRecorder_Initialize(void *recorder)
{
    (void)recorder;
    return 0;
}

void CAudioRecorder_Shutdown(void *recorder)
{
    (void)recorder;
}

void *CAudioRecorder_GetBuffer(void *recorder, unsigned int *outSize)
{
    (void)recorder;
    if (outSize)
        *outSize = 0;
    return 0;
}

void CAudioRecorder_SetRecordLevel(void *recorder, float level)
{
    (void)recorder;
    (void)level;
}

void CSoundObject_get_sample_volume_pan(void *obj, float *volume, float *pan)
{
    (void)obj;
    if (volume)
        *volume = 0.0f;
    if (pan)
        *pan = 0.0f;
}

void CSoundObject_get_sample_volume_levels(void *obj, float *leftLevel, float *rightLevel)
{
    (void)obj;
    if (leftLevel)
        *leftLevel = 0.0f;
    if (rightLevel)
        *rightLevel = 0.0f;
}

unsigned long CSoundObject_get_sample_position(void *obj)
{
    (void)obj;
    return 0;
}

unsigned long CSoundObject_get_sample_length(void *obj)
{
    (void)obj;
    return 0;
}

float CSoundObject_get_sample_volume(void *obj)
{
    (void)obj;
    return 0.0f;
}

long CSoundObject_set_sample_info(void *obj, const void *info)
{
    (void)obj;
    (void)info;
    return 0;
}

void CSoundObject_CSoundObject(void *obj, void *engine, unsigned int busIndex)
{
    (void)engine;
    (void)busIndex;
    (void)obj;
}

void CSoundObject_get_3D_position(void *obj, float *x, float *y, float *z)
{
    (void)obj;
    if (x)
        *x = 0.0f;
    if (y)
        *y = 0.0f;
    if (z)
        *z = 0.0f;
}

void DSound_Frame(void)
{
}

void DSound_SampleFrame(void *sample)
{
    (void)sample;
}

int DSound_Init(int numChannels, const unsigned char *handle)
{
    (void)numChannels;
    (void)handle;
    return 0;
}

void DSound_Shutdown(void)
{
}
