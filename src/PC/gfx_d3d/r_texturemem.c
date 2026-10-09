#include "common_types.h"
#include "imports.h"

extern void MacDisplay_GetVideoMemoryInfo(int *vidMemMB, int *totalVidMem);

extern unsigned char dx[];
extern refimport_t ri;

unsigned int R_AvailableTextureMemory(void)
{
    int vidMem;
    int textureMemBytes;
    unsigned int texMemInMegs;

    MacDisplay_GetVideoMemoryInfo(&vidMem, &textureMemBytes);
    vidMem >>= 20;

    char *device = *(char **)(dx + 8);
    void **vtable = *(void ***)device;
    texMemInMegs = ((unsigned int(D3DVTCC *)(void *))vtable[4])(device) >> 20;

    if (vidMem == 0) {
        ri.Printf(0, "DirectX reports %i MB of available texture memory, but wouldn't tell available video memory.\n", texMemInMegs);
    } else {
        ri.Printf(0, "DirectX reports %i MB of video memory and %i MB of available texture memory.\n", vidMem, texMemInMegs);
        if ((unsigned int)vidMem < texMemInMegs) {
            texMemInMegs = vidMem - 16;
            ri.Printf(0, "Using video memory size to cap used texture memory at %i MB.\n", texMemInMegs);
        }
    }
    return texMemInMegs;
}
