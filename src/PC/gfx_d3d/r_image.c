#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern int alwaysfails;
extern DxGlobals dx;
extern refimport_t ri;

/* g_platform_name: migrated from the ILP32 data blob to typed C (re-lays-out per
 * target; trailing blob bytes were inter-symbol padding). (x64 port Stage 2.) */
const char *g_platform_name[2] = {
    "current", " min_pc",
};
/* Image registry: x86 packs a GfxImage* hash table [0..2047] + scalar fields
 * (picmip/cardMemory) [2048..] into an int[2064] (4-byte slots). On x64 the
 * pointer table must widen, so it's a typed struct (runtime, layout-free).
 * Access via IG_* (x64 typed, x86 byte-identical raw). */
#if defined(COD2_X64)
struct ImageGlobals {
    GfxImage *hashTable[2048];
    int scalars[16];           /* old indices 2048..2063 */
};
static struct ImageGlobals imageGlobalsS;
#define IG_HASH(h)     (imageGlobalsS.hashTable[h])
#define IG_SCALAR(i)   (imageGlobalsS.scalars[(i) - 2048])
#define IG_CLEARHASH() memset(imageGlobalsS.hashTable, 0, sizeof(imageGlobalsS.hashTable))
#else
static int imageGlobals[2064];
#define IG_HASH(h)     (*(GfxImage **)&imageGlobals[h])
#define IG_SCALAR(i)   (imageGlobals[i])
#define IG_CLEARHASH() memset(imageGlobals, 0, 0x2000)
#endif
static GfxImage g_imageProgs[12];

__attribute__((used, packed, aligned(4)))
const char *imageTypeName[] = {
    "misc  ",
    "debug ",
    "$tex+?",
    "ui    ",
    "lmap  ",
    "light ",
    "f/x   ",
    "hud   ",
    "model ",
    "world ",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
__attribute__((used, packed, aligned(4)))
const char *g_imageProgNames[] = {
    "$dynamic_shadows",
    "$shadow_cookie",
    "$shadow_cookie_blur",
    "$blurred_screen",
    "$glow_0",
    "$glow_1",
    "$pingpong_0",
    "$pingpong_1",
    "$resolved_post_sun",
    "$resolved_scene",
    "$savedscreen",
    "$raw",
    NULL,
    NULL,
    NULL,
    NULL,
};

static void R_AddImageToList(union XAssetHeader header, void *data);
extern void DB_EnumXAssets(int type, void (*func)(union XAssetHeader, void *), void *data, int overrides);
extern Bool Image_LoadFromFile(GfxImage *image);
extern void R_Error(int level, const char *msg, ...);
void R_GetImageList(ImageList *imageList);
int R_GetMinSpecImageMemory(void);
void R_ResetImageAllocations(void);
void R_FreeImageAllocations(void);
void Image_Create2DTexture(GfxImage *image, int width, int height, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_Create3DTexture(GfxImage *image, int width, int height, int depth, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_CreateCubeTexture(GfxImage *image, int edgeLen, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_PicmipForSemantic(unsigned char semantic, Picmip *picmip);
IDirect3DSurface9 *Image_GetSurface(GfxImage *image);
void R_SetPicmip(void);
static int imagecompare(GfxImage *image1, GfxImage *image2);
water_t *R_LoadWaterSetup(const water_t *water);
void R_DownsampleMipMapBilinear(const byte *src, int srcBufferSize, int srcWidth, int srcHeight, int texelPitch, byte *dst, int dstBufferSize);
void Image_Release(GfxImage *image);
void R_ReloadLostImages(void);
void Image_TrackTexture(GfxImage *image, int imageFlags, D3DFORMAT format, int width, int height, int depth);
void Image_TrackFullscreenTexture(GfxImage *image, int picmip, D3DFORMAT format);
GfxImage *Image_AllocProg(int imageProgType, int category);
GfxImage *Image_Alloc(const char *name, int category, int semantic, int imageTrack);
void R_ImageList_f(void);
GfxImage *Image_Register(const char *imageName, int semantic, int imageTrack);
void R_InitImages(void);
void Image_SetupRenderTarget(GfxImage *image, int width, int height, D3DFORMAT imageFormat);
void Image_SetupSystem(GfxImage *image, int width, int height, D3DFORMAT imageFormat);
void Image_RebuildCosinePowerMap(float shift);
void R_ShutdownImages(void);
void R_ReleaseLostImages(void);
void Image_UpdatePicmip(GfxImage *image);
void Image_Reload(GfxImage *image);
typedef int (*GfxImageCompFunc)(GfxImage *, GfxImage *);
void ZSt13__adjust_heapIPP8GfxImageiS1_PFiS1_S1_EEvT_T0_S6_T1_T2_(GfxImage **first, int holeIndex, int len, GfxImage *value, GfxImageCompFunc comp);
void ZSt16__insertion_sortIPP8GfxImagePFiS1_S1_EEvT_S5_T0_(GfxImage **first, GfxImage **last, GfxImageCompFunc comp);
void ZSt16__introsort_loopIPP8GfxImageiPFiS1_S1_EEvT_S5_T0_T1_(GfxImage **first, GfxImage **last, int depth_limit, GfxImageCompFunc comp);

static void R_AddImageToList(union XAssetHeader header, void *data)
{
    ImageList *list = (ImageList *)data;
    list->image[list->count++] = (GfxImage *)header.data;
}

void R_GetImageList(ImageList *imageList)
{
    imageList->count = 0;
    DB_EnumXAssets(3, R_AddImageToList, imageList, 1);
}

int R_GetMinSpecImageMemory(void)
{
    return IG_SCALAR(2052);
}

void R_ResetImageAllocations(void)
{
}

void R_FreeImageAllocations(void)
{
}

extern const char *R_ErrorDescription(HRESULT hr);

static inline __attribute__((always_inline)) void Image_Create2DTexture_core(GfxImage *image, int width, int height, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool)
{
    byte *img = (byte *)image;
    void *device;
    void **vtable;
    HRESULT hr;

    image->width = (unsigned short)width;
    image->height = (unsigned short)height;
    image->depth = 1;
    image->mapType = 3;

    device = dx.device;
    vtable = *(void ***)device;
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(vtable[0x5C / 4]))(
        device, (unsigned short)width, (unsigned short)height,
        mipmapCount, usage, imageFormat, memPool, (void **)&image->texture.map, NULL);

    if (hr < 0) {
        R_Error(1, "Create2DTexture( %s, %i, %i, %i, %i ) failed: %08x = %s",
                image->name,
                (int)image->width,
                (int)image->height,
                0, (int)imageFormat, (int)hr, R_ErrorDescription(hr));
    }
}

void Image_Create2DTexture(GfxImage *image, int width, int height, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool)
{
    Image_Create2DTexture_core(image, width, height, mipmapCount, usage, imageFormat, memPool);
}

void Image_Create3DTexture(GfxImage *image, int width, int height, int depth, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool)
{
    byte *img = (byte *)image;
    void *device;
    void **vtable;
    HRESULT hr;

    image->width = (unsigned short)width;
    image->height = (unsigned short)height;
    image->depth = (unsigned short)depth;
    image->mapType = 4;

    device = dx.device;
    if (!device) {
        image->texture.map = NULL;
        return;
    }
    vtable = *(void ***)device;
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(vtable[0x60 / 4]))(
        device, (unsigned short)width, (unsigned short)height, (unsigned short)depth,
        mipmapCount, usage, imageFormat, memPool, (void **)&image->texture.map, NULL);

    if (hr < 0) {
        R_Error(1, "Create3DTexture( %s, %i, %i, %i, %i, %i ) failed: %08x = %s",
                image->name,
                (int)image->width,
                (int)image->height,
                (int)image->depth,
                0, (int)imageFormat, (int)hr, R_ErrorDescription(hr));
    }
}

void Image_CreateCubeTexture(GfxImage *image, int edgeLen, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool)
{
    DxGlobals *dxg = (DxGlobals *)imp_dx;
    byte *img = (byte *)image;
    byte *dx;
    void *device;
    void **vtable;
    HRESULT hr;
    int actualMipCount;

    image->width = (unsigned short)edgeLen;
    image->height = (unsigned short)edgeLen;
    image->depth = 1;
    image->mapType = 5;

    if (dxg->canMipCubemaps)
        actualMipCount = mipmapCount;
    else
        actualMipCount = 1;

    device = dxg->device;
    if (!device) {
        image->texture.map = NULL;
        return;
    }
    vtable = *(void ***)device;
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(vtable[0x64 / 4]))(
        device, (unsigned short)edgeLen, actualMipCount, 0, imageFormat, memPool,
        (void **)&image->texture.map, NULL);

    if (hr < 0) {
        R_Error(1, "CreateCubeTexture ( %s, %i, %i, %i ) failed: %08x = %s",
                image->name,
                (int)image->width,
                mipmapCount, (int)imageFormat, (int)hr, R_ErrorDescription(hr));
    }
}

void Image_PicmipForSemantic(unsigned char semantic, Picmip *picmip)
{
    byte s = semantic;
    int val;

    switch (s) {
    case 2:
    case 5:
        val = IG_SCALAR(2048);
        break;
    case 3:
        val = IG_SCALAR(2049);
        break;
    case 4:
        val = IG_SCALAR(2050);
        break;
    default:
        *(unsigned short *)picmip = 0;
        return;
    }

    ((byte *)picmip)[1] = 2;
    if (val < 0)
        ((byte *)picmip)[0] = 0;
    else
        ((byte *)picmip)[0] = (byte)(val < 4 ? val : 3);
}

IDirect3DSurface9 *Image_GetSurface(GfxImage *image)
{
    IDirect3DSurface9 *surface;
    void *texture;
    void **vtable;
    volatile int *fails = (volatile int *)&alwaysfails;

    do {
        texture = image->texture.map;
        vtable = *(void ***)texture;
        ((HRESULT(D3DVTCC *)(void *, UINT, IDirect3DSurface9 **))vtable[0x48 / 4])(texture, 0, &surface);
    } while (*fails);

    return surface;
}

extern int R_AvailableTextureMemory(void);

void R_SetPicmip(void)
{
    void (*ri_Printf)(int, const char *, ...) = *(void (**)(int, const char *, ...))&ri;
    void (*Cvar_SetValue)(void *, int) = (void (*)(void *, int))ri.Dvar_SetInt;
    int (*Cvar_VariableIntegerValue)(const char *) = (int (*)(const char *))ri.Dvar_GetInt;
    int texMemInMegs, sysMemInMegs;
    int minPicmip;
    int changed;

    texMemInMegs = R_AvailableTextureMemory();
    sysMemInMegs = Cvar_VariableIntegerValue("sys_sysMB");

    if ((*(const dvar_t **)imp_r_picmip_manual)->current.enabled) {
        ri_Printf(0, "Using manual picmip settings\n");
        IG_SCALAR(2048) = (*(const dvar_t **)imp_r_picmip)->current.integer;
        IG_SCALAR(2049) = (*(const dvar_t **)imp_r_picmip_bump)->current.integer;
        IG_SCALAR(2050) = (*(const dvar_t **)imp_r_picmip_spec)->current.integer;
    } else if (r_rendererInUse->current.integer == 2) {

        ri_Printf(0, "Dx7 renderer: using low-res textures\n");
        if (texMemInMegs > 128) {
#ifdef __EMSCRIPTEN__
            IG_SCALAR(2048) = 0;
            IG_SCALAR(2049) = 0;
            IG_SCALAR(2050) = 0;
#else
            IG_SCALAR(2048) = 1;
            IG_SCALAR(2049) = 1;
            IG_SCALAR(2050) = 1;
#endif
        } else {
            IG_SCALAR(2048) = 2;
            IG_SCALAR(2049) = 2;
            IG_SCALAR(2050) = 2;
        }

        if (sysMemInMegs <= 383) {
            changed = 0;
            if (IG_SCALAR(2048) < 1) {
                IG_SCALAR(2048) = 1;
                changed = 1;
            }
            if (IG_SCALAR(2049) < 1) {
                IG_SCALAR(2049) = 1;
                changed = 1;
            }
            if (IG_SCALAR(2050) < 1) {
                IG_SCALAR(2050) = 1;
                changed = 1;
            }
            if (changed)
                ri_Printf(0, "Reducing texture detail based on total system memory of %i M", sysMemInMegs);
        }
    } else {

        ri_Printf(0, "Using non-Dx7 renderer\n");

        IG_SCALAR(2048) = 2;
        IG_SCALAR(2049) = 2;
        IG_SCALAR(2050) = 2;

        if (sysMemInMegs <= 383) {
            if (sysMemInMegs <= 479) {
                minPicmip = 1;
                goto apply_sysmem;
            }
        } else {
            minPicmip = 2;
        apply_sysmem:
            changed = 0;
            if (IG_SCALAR(2048) < minPicmip) {
                IG_SCALAR(2048) = minPicmip;
                changed = 1;
            }
            if (IG_SCALAR(2049) < minPicmip) {
                IG_SCALAR(2049) = minPicmip;
                changed = 1;
            }
            if (IG_SCALAR(2050) < minPicmip) {
                IG_SCALAR(2050) = minPicmip;
                changed = 1;
            } else if (!changed) {
                goto set_cvars;
            }
            ri_Printf(0, "Reducing texture detail based on total system memory of %i M", sysMemInMegs);
        }
    }

set_cvars:

    Cvar_SetValue = (void (*)(void *, int))ri.Dvar_SetInt;
    ri_Printf = *(void (**)(int, const char *, ...))&ri;
    Cvar_SetValue(*(const dvar_t **)imp_r_picmip, IG_SCALAR(2048));
    Cvar_SetValue(*(const dvar_t **)imp_r_picmip_bump, IG_SCALAR(2049));
    Cvar_SetValue(*(const dvar_t **)imp_r_picmip_spec, IG_SCALAR(2050));
    ri_Printf(0, "Using picmip %i on most textures, %i on normal maps, and %i on spec maps",
              IG_SCALAR(2048), IG_SCALAR(2049), IG_SCALAR(2050));
}

static int imagecompare(GfxImage *image1, GfxImage *image2)
{
    if (image1->track > image2->track)
        return 0;
    if (image1->track < image2->track)
        return 1;
    return image1->cardMemory.platform[0] < image2->cardMemory.platform[0];
}

extern void Com_Printf(const char *fmt, ...);
extern int R_WatersEquivalent(const water_t *a, const water_t *b);
extern void R_CreateWaterSetup(const water_t *water, int index, water_t *dest);

water_t *R_LoadWaterSetup(const water_t *water)
{
    byte *rg = (byte *)imp_rg;
    int waterCount = ((r_globals_t *)rg)->sceneWaterMapSetupsCount;
    int i;
    water_t *slot;

    for (i = 0; i < waterCount; i++) {
        rg = (byte *)imp_rg;

        slot = &((r_globals_t *)rg)->sceneWaterMapSetups[i];
        if (R_WatersEquivalent(slot, water))
            return slot;
    }

    if (i == 16) {
        Com_Printf("^1ERROR: map uses more than %i waterMap textures\n", 16);
        return NULL;
    }

    rg = (byte *)imp_rg;
    slot = &((r_globals_t *)rg)->sceneWaterMapSetups[i];
    R_CreateWaterSetup(water, i, slot);
    ((r_globals_t *)rg)->sceneWaterMapSetupsCount += 1;

    return slot;
}

void R_DownsampleMipMapBilinear(const byte *src, int srcBufferSize, int srcWidth, int srcHeight, int texelPitch, byte *dst, int dstBufferSize)
{
    int dstWidth, dstHeight;
    int srcRowPitch, dstRowPitch;
    int row, col, ch;

    (void)srcBufferSize;
    (void)dstBufferSize;

    if (srcWidth == 1 && srcHeight == 1)
        return;

    dstWidth = srcWidth >> 1;
    dstHeight = srcHeight >> 1;
    srcRowPitch = srcWidth * texelPitch;
    dstRowPitch = dstWidth * texelPitch;

    if (dstWidth == 0 || dstHeight == 0) {
        int totalTexels = dstWidth + dstHeight;
        const byte *srcPos = src;
        byte *dstPos = dst;
        int srcStride = texelPitch * 2;

        for (col = 0; col < totalTexels; col++) {
            for (ch = 0; ch < texelPitch; ch++) {

                dstPos[ch] = (byte)(((int)srcPos[ch] + (int)srcPos[ch + texelPitch]) >> 1);
            }
            dstPos += texelPitch;
            srcPos += srcStride;
        }
        return;
    }

    for (row = 0; row < dstHeight; row++) {
        const byte *srcRow = src + row * 2 * srcRowPitch;
        byte *dstRow = dst + row * dstRowPitch;

        for (col = 0; col < dstWidth; col++) {
            const byte *srcTexel = srcRow;
            const byte *srcTexelNextRow = srcRow + texelPitch + srcRowPitch;

            for (ch = 0; ch < texelPitch; ch++) {

                int sum = (int)srcTexel[ch];
                sum += (int)srcTexel[ch + texelPitch];
                sum += (int)srcTexel[ch + srcRowPitch];
                sum += (int)srcTexelNextRow[0];
                dstRow[ch] = (byte)(sum >> 2);
                srcTexelNextRow++;
            }
            dstRow += texelPitch;
            srcRow += texelPitch * 2;
        }
    }
}

static inline __attribute__((always_inline)) void Image_Release_core(GfxImage *image)
{
    byte *img = (byte *)image;
    void *texture;
    void **vtable;
    int i;

    if (*(unsigned char *)&image->track > 4 ||
        !((1 << *(signed char *)&image->track) & 0x13)) {

        for (i = 0; i < 2; i++) {
            IG_SCALAR(0x200c / 4 + i) -= image->cardMemory.platform[i];
        }
    }

    texture = image->texture.map;
    if (texture) {
        vtable = *(void ***)texture;

        if (vtable && vtable[8 / 4])
            ((ULONG(D3DVTCC *)(void *))(vtable[8 / 4]))(texture);
        image->texture.map = NULL;
        image->cardMemory.platform[0] = 0;
        image->cardMemory.platform[1] = 0;
    }

    image->mapType = 0;
}

void Image_Release(GfxImage *image)
{
    Image_Release_core(image);
}

extern void Image_BuildWaterMap(GfxImage *image);

void R_ReloadLostImages(void)
{
    int i;

    for (i = 0; i < 2048; i++) {
        GfxImage *image = IG_HASH(i);
        byte category;
        int isProg;

        if (!image)
            continue;

        category = image->category;
        if (category <= 4)
            continue;

        if (image->texture.map != NULL)
            continue;

        isProg = ((char *)image >= (char *)g_imageProgs &&
                  (char *)image < (char *)g_imageProgs + sizeof(g_imageProgs));

        if (isProg)
            continue;

        if (category != 5)
            continue;

        Image_BuildWaterMap(image);
    }
}

extern int Image_GetCardMemoryAmount(int imageFlags, D3DFORMAT format, int width, int height, int depth);

void Image_TrackTexture(GfxImage *image, int imageFlags, D3DFORMAT format, int width, int height, int depth)
{
    byte *img = (byte *)image;
    signed char mapType = (signed char)image->track;
    int needsGlobalAccounting;
    int amount;
    int i;

    needsGlobalAccounting = ((byte)mapType > 4) || !((1 << mapType) & 0x13);

    if (!(imageFlags & 1)) {

        for (i = 0; i < 2; i++) {
            int picmipLevel = image->picmip.platform[i];
            int mipW = width >> picmipLevel;
            int mipH = height >> picmipLevel;
            int mipD = depth >> picmipLevel;
            if (mipW < 1)
                mipW = 1;
            if (mipH < 1)
                mipH = 1;
            if (mipD < 1)
                mipD = 1;

            amount = Image_GetCardMemoryAmount(imageFlags, format, mipW, mipH, mipD);
            image->cardMemory.platform[i] = amount;

            if (needsGlobalAccounting)
                IG_SCALAR(0x200c / 4 + i) += amount;
        }
    } else {

        for (i = 0; i < 2; i++) {
            amount = Image_GetCardMemoryAmount(imageFlags, format, width, height, depth);
            image->cardMemory.platform[i] = amount;

            if (needsGlobalAccounting)
                IG_SCALAR(0x200c / 4 + i) += amount;
        }
    }
}

void Image_TrackFullscreenTexture(GfxImage *image, int picmip, D3DFORMAT format)
{
    byte *img = (byte *)image;
    signed char mapType = (signed char)image->track;
    int needsGlobalAccounting = ((byte)mapType > 4) || !((1 << mapType) & 0x13);
    int platform;
    int screenWidth, screenHeight;
    int mipW, mipH;
    int amount;

    for (platform = 0; platform <= 1; platform++) {
        if (platform == 0) {

            int *vidConfig = (int *)imp_vidConfig;
            screenWidth = vidConfig[0];
            screenHeight = vidConfig[1];
        } else {

            screenWidth = 640;
            screenHeight = 480;
        }

        mipW = screenWidth >> (byte)picmip;
        if (mipW < 1)
            mipW = 1;
        mipH = screenHeight >> (byte)picmip;
        if (mipH < 1)
            mipH = 1;

        amount = Image_GetCardMemoryAmount(3, format, mipW, mipH, 1);
        image->cardMemory.platform[platform] += amount;

        if (needsGlobalAccounting)
            IG_SCALAR(0x200c / 4 + platform) += amount;
    }
}

extern int R_HashAssetName(const char *name);

GfxImage *Image_AllocProg(int imageProgType, int category)
{

    GfxImage *image = &g_imageProgs[imageProgType];
    const char *name = g_imageProgNames[imageProgType];
    int hash;

    image->name = name;
    image->category = (byte)category;
    image->semantic = 0;
    image->track = 0;

    hash = R_HashAssetName(name) & 0x7ff;
    while (IG_HASH(hash) != 0) {
        hash = (hash + 1) & 0x7ff;
    }
    IG_HASH(hash) = image;

    return image;
}

GfxImage *Image_Alloc(const char *name, int category, int semantic, int imageTrack)
{
    int nameLen = strlen(name) + 1;
    void *(*hunkAlloc)(int) = ri.Hunk_AllocInternal;
    byte *image;
    char *nameDst;
    int hash;

    image = (byte *)hunkAlloc(0x24 + nameLen);

    nameDst = (char *)(image + sizeof(GfxImage));
    ((GfxImage *)image)->name = nameDst;
    memcpy(nameDst, name, nameLen);

    ((GfxImage *)image)->category = (byte)category;
    ((GfxImage *)image)->semantic = (byte)semantic;
    ((GfxImage *)image)->track = (byte)imageTrack;

    hash = R_HashAssetName(name) & 0x7ff;
    while (IG_HASH(hash) != 0) {
        hash = (hash + 1) & 0x7ff;
    }
    IG_HASH(hash) = image;

    return (GfxImage *)image;
}

extern int I_stricmp(const char *, const char *);
void R_ImageList_f(void)
{
    typedef void (*PrintFunc)(int, const char *, ...);
    typedef int (*CmdArgcFunc)(void);
    typedef const char *(*CmdArgvFunc)(int);
    typedef HRESULT(__attribute__((stdcall)) * GetDescFunc)(void *, UINT, void *);

    PrintFunc Com_Printf = *(PrintFunc *)((char *)&ri);
    CmdArgcFunc Cmd_Argc = (CmdArgcFunc)ri.Cmd_Argc;
    CmdArgvFunc Cmd_Argv = (CmdArgvFunc)ri.Cmd_Argv;

    ImageList imageListBuf;
    int imageTrack[20];
    int total[2];
    byte listAllImages;
    int i, j, platform;
    int desc[8];

    if (Cmd_Argc() == 2) {
        const char *arg = Cmd_Argv(1);
        listAllImages = (I_stricmp(arg, "all") == 0) ? 1 : 0;
    } else {
        listAllImages = 0;
    }

    total[0] = 0;
    total[1] = 0;
    memset(imageTrack, 0, sizeof(imageTrack));

    imageListBuf.count = 0;
    DB_EnumXAssets(3, R_AddImageToList, &imageListBuf, 1);

    if (listAllImages) {
        int count;
        for (j = 0; j < 12; j++) {
            count = imageListBuf.count;
            if ((unsigned int)count > 0x7ff)
                break;
            if (g_imageProgs[j].mapType != 0) {
                imageListBuf.image[count] = &g_imageProgs[j];
                imageListBuf.count = count + 1;
            }
        }
    }

    {
        int count = imageListBuf.count;
        GfxImage **first = imageListBuf.image;
        GfxImage **last = first + count;

        if (first != last) {
            int n = count;
            int depth = 0;
            if (n > 1) {
                int tmp = n;
                while (tmp > 1) {
                    depth++;
                    tmp >>= 1;
                }
                depth *= 2;
            }
            ZSt16__introsort_loopIPP8GfxImageiPFiS1_S1_EEvT_S5_T0_T1_(first, last, depth, imagecompare);
            if ((char *)last - (char *)first > 0x43) {
                GfxImage **threshold = first + 16;
                ZSt16__insertion_sortIPP8GfxImagePFiS1_S1_EEvT_S5_T0_(first, threshold, imagecompare);

                for (i = (int)(threshold - first); &first[i] != last; i++) {
                    GfxImage *val = first[i];
                    GfxImage **hole = &first[i];
                    GfxImage **prev = hole - 1;
                    while (imagecompare(val, *prev)) {
                        *hole = *prev;
                        hole = prev;
                        prev--;
                    }
                    *hole = val;
                }
            } else {
                ZSt16__insertion_sortIPP8GfxImagePFiS1_S1_EEvT_S5_T0_(first, last, imagecompare);
            }
        }

        Com_Printf(0, "\n-if-- ");
        for (j = 0; j < 2; j++)
            Com_Printf(0, "%s", g_platform_name[j]);
        Com_Printf(0, " ---------\n");

        for (i = 0; i < count; i++) {
            GfxImage *image = first[i];
            int imageKind = image->mapType;
            int format;
            void *d3dRes;

            if (imageKind == 4) {

                d3dRes = image->texture.map;
                (*(GetDescFunc **)d3dRes)[0x44 / 4](d3dRes, 0, desc);
                format = desc[0];
            } else if (imageKind == 5 || imageKind == 3) {

                d3dRes = image->texture.map;
                (*(GetDescFunc **)d3dRes)[0x44 / 4](d3dRes, 0, desc);
                format = desc[0];
            } else {

                return;
            }

            switch (format) {
            case 0x15:
                Com_Printf(0, "A1R5");
                break;
            case 0x16:
                Com_Printf(0, "A4R4");
                break;
            case 0x17:
                Com_Printf(0, "R3G3");
                break;
            case 0x1c:
                Com_Printf(0, "8888");
                break;
            case 0x32:
                Com_Printf(0, "L___");
                break;
            case 0x33:
                Com_Printf(0, "AL__");
                break;
            case 0x31545844:
                Com_Printf(0, "DXT1");
                break;
            case 0x33545844:
                Com_Printf(0, "DXT3");
                break;
            case 0x35545844:
                Com_Printf(0, "DXT5");
                break;
            case 0x72:
                Com_Printf(0, "DP__");
                break;
            default:
                break;
            }

            Com_Printf(0, "  %s", imageTypeName[image->track]);

            if (!listAllImages) {

                for (platform = 0; platform < 2; platform++) {
                    int size = image->cardMemory.platform[platform];
                    float sizeKB = (float)size * 0.0009765625f;
                    if (sizeKB < 10.0f)
                        Com_Printf(0, "%7.1f", (double)sizeKB);
                    else
                        Com_Printf(0, "%7.0f", (double)sizeKB);

                    {
                        int imgType = image->track;
                        imageTrack[imgType * 2 + platform] += size;
                    }

                    {
                        int imgType = image->track;
                        if (imgType <= 4 && ((1 << imgType) & 0x13))
                            ;
                        else
                            total[platform] += size;
                    }
                }
            } else {

                for (platform = 0; platform < 2; platform++) {
                    int size = image->cardMemory.platform[platform];
                    float sizeKB = (float)size * 0.0009765625f;
                    const char *fmt = (sizeKB >= 10.0f) ? "%7.0f" : "%7.1f";
                    Com_Printf(0, fmt, (double)sizeKB);

                    {
                        int imgType = image->track;
                        imageTrack[imgType * 2 + platform] += size;
                    }
                    total[platform] += size;
                }
            }

            Com_Printf(0, "  %s\n", image->name);
        }

        Com_Printf(0, " ---------\n");
        Com_Printf(0, " %i total images\n", count);

        for (platform = 0; platform < 2; platform++) {
            Com_Printf(0, " %5.1f MB %s total image size\n",
                       (double)((float)total[platform] * 9.5367431640625e-07f),
                       g_platform_name[platform]);
        }
        Com_Printf(0, "\n");
        Com_Printf(0, " ---------\n");
        for (j = 0; j < 2; j++)
            Com_Printf(0, "%s", g_platform_name[j]);
        Com_Printf(0, "\n");

        for (j = 0; j < 10; j++) {
            Com_Printf(0, "%s:", imageTypeName[j]);
            for (platform = 0; platform < 2; platform++) {
                Com_Printf(0, "  %5.1f",
                           (double)((float)imageTrack[j * 2 + platform] * 9.5367431640625e-07f));
            }
            Com_Printf(0, "\n");
        }
        Com_Printf(0, "\n");
    }
}

extern GfxImage *Image_Load(const char *name, int semantic, int imageTrack);

GfxImage *Image_Register(const char *imageName, int semantic, int imageTrack)
{
    int hash;
    GfxImage *image;
    int isProg;

    hash = R_HashAssetName(imageName) & 0x7ff;
    image = IG_HASH(hash);

    while (image) {
        if (strcmp(imageName, image->name) == 0)
            break;
        hash = (hash + 1) & 0x7ff;
        image = IG_HASH(hash);
    }

    isProg = ((char *)image >= (char *)g_imageProgs &&
              (char *)image < (char *)g_imageProgs + sizeof(g_imageProgs));

    if (!isProg && image)
        return image;

    image = Image_Load(imageName, (byte)semantic, imageTrack);
    if (!image) {
        R_Error(1, "failed to load image '%s'", imageName);
    }

    return image;
}

extern void RB_InitImages(void);

void R_InitImages(void)
{
    r_global_permanent_t *rgp;
    int rendererType;
    int hash;
    GfxImage *rawImage;

    R_SetPicmip();

    {
        GfxImage *whiteImage = Image_Register("$white", 1, 0);
        rgp = (r_global_permanent_t *)imp_rgp;
        rgp->whiteImage = whiteImage;
    }
    rgp->blackImage = Image_Register("$black", 1, 0);

    rendererType = r_rendererInUse->current.integer;
    if (rendererType == 2) {

        rgp->waterColorImage = Image_Register("$watercolor", 1, 0);
    } else {

        rgp->identityNormalMapImage = Image_Register("$identitynormalmap", 1, 0);
        rgp->specularityImage = Image_Register("$specularity", 1, 0);
        rgp->lightGridWeightsImage[0] = Image_Register("$lightgridweights0", 1, 0);
        rgp->lightGridWeightsImage[1] = Image_Register("$lightgridweights1", 1, 0);
        rgp->lightmapWeightsImage = Image_Register("$lightmapweights", 1, 0);
    }

    RB_InitImages();

    rawImage = &g_imageProgs[11];
    rawImage->name = g_imageProgNames[11];
    rawImage->category = 4;
    rawImage->semantic = 0;
    rawImage->track = 0;

    hash = R_HashAssetName(g_imageProgNames[11]) & 0x7ff;
    while (IG_HASH(hash) != 0)
        hash = (hash + 1) & 0x7ff;
    IG_HASH(hash) = rawImage;

    rgp = (r_global_permanent_t *)imp_rgp;
    rgp->rawImage = rawImage;
    rgp->rawTexdef.samplerState = 0x32;
    rgp->rawTexdef.semantic = 0;
    rgp->rawTexdef.u.image = rawImage;
}

void Image_SetupRenderTarget(GfxImage *image, int width, int height, D3DFORMAT imageFormat)
{
    unsigned short w = (unsigned short)width;
    unsigned short h = (unsigned short)height;
    void *device;
    void **vtable;
    HRESULT hr;

    image->width = w;
    image->height = h;
    image->depth = 1;
    image->mapType = 3;

    device = dx.device;
    if (!device) {
        image->texture.map = NULL;
        return;
    }
    vtable = *(void ***)device;
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(vtable[0x5C / 4]))(
        device, w, h, 1, 1, imageFormat, 0, (void **)&image->texture.map, NULL);

    if (hr < 0) {
        R_Error(1, "Create2DTexture( %s, %i, %i, %i, %i ) failed: %08x = %s",
                image->name, (int)w, (int)h, 0, (int)imageFormat,
                (int)hr, R_ErrorDescription(hr));
    }

    Image_TrackTexture(image, 3, imageFormat, w, h, 1);
}

void Image_SetupSystem(GfxImage *image, int width, int height, D3DFORMAT imageFormat)
{
    unsigned short w = (unsigned short)width;
    unsigned short h = (unsigned short)height;
    void *device;
    void **vtable;
    HRESULT hr;

    image->width = w;
    image->height = h;
    image->depth = 1;
    image->mapType = 3;

    device = dx.device;
    if (!device) {
        image->texture.map = NULL;
        return;
    }
    vtable = *(void ***)device;
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(vtable[0x5C / 4]))(
        device, w, h, 1, 0x200, imageFormat, 2, (void **)&image->texture.map, NULL);

    if (hr < 0) {
        R_Error(1, "Create2DTexture( %s, %i, %i, %i, %i ) failed: %08x = %s",
                image->name, (int)w, (int)h, 0, (int)imageFormat,
                (int)hr, R_ErrorDescription(hr));
    }

    Image_TrackTexture(image, 3, imageFormat, w, h, 1);
}

extern void RB_UnbindImage(const GfxImage *image);
extern void Image_BuildSpecularityMap(float shift, byte *pic);
extern void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);

void Image_RebuildCosinePowerMap(float shift)
{
    r_global_permanent_t *rgp;
    GfxImage *image;
    void *texture;
    void **vtable;
    byte pic[0x2000];

    if (r_rendererInUse->current.integer == 2)
        return;

    rgp = (r_global_permanent_t *)imp_rgp;
    image = rgp->specularityImage;
    RB_UnbindImage(image);

    do {
        texture = image->texture.map;
        vtable = *(void ***)texture;
        ((ULONG(D3DVTCC *)(void *))(vtable[8 / 4]))(texture);
        image->texture.map = NULL;
    } while (*(volatile int *)&alwaysfails);

    image = rgp->specularityImage;
    Image_Create2DTexture_core(image, 32, 256, 1, 0, 0x32, 1);

    Image_BuildSpecularityMap(shift, pic);
    Image_UploadData(rgp->specularityImage, 0x32, 0, 0, pic);
}

extern void RB_UnbindAllImages(void);

void R_ShutdownImages(void)
{
    GfxImage *savedImages[2048];
    int savedImageCount = 0;
    int i;
    int hash;

    RB_UnbindAllImages();

    for (i = 0; i < 2048; i++) {
        GfxImage *image = IG_HASH(i);
        int isProg;

        if (!image)
            continue;

        isProg = ((char *)image >= (char *)g_imageProgs) &
                 ((char *)image < (char *)g_imageProgs + sizeof(g_imageProgs));

        if (isProg) {

            savedImages[savedImageCount++] = image;
            continue;
        }

        Image_Release_core(image);
    }

    IG_CLEARHASH();

    for (i = 0; i < savedImageCount; i++) {
        GfxImage *image = savedImages[i];
        const char *name = image->name;

        hash = R_HashAssetName(name) & 0x7ff;
        while (IG_HASH(hash) != 0)
            hash = (hash + 1) & 0x7ff;
        IG_HASH(hash) = image;
    }
}

void R_ReleaseLostImages(void)
{
    int i;

    for (i = 0; i < 2048; i++) {
        GfxImage *image = IG_HASH(i);

        if (!image)
            continue;

        if (image->category <= 4)
            continue;

        Image_Release_core(image);
    }
}

extern void Image_GetPicmip(const GfxImage *image, Picmip *picmip);

void Image_UpdatePicmip(GfxImage *image)
{
    Picmip picmip;

    Image_GetPicmip(image, &picmip);

    if (image->picmip.platform[0] == picmip.platform[0])
        return;

    Image_Release_core(image);

    image->picmip = picmip;

    if (!Image_LoadFromFile(image)) {
        R_Error(1, "failed to load image '%s'",
                image->name);
    }
}

void Image_Reload(GfxImage *image)
{
    Image_Release_core(image);

    if (!Image_LoadFromFile(image)) {
        R_Error(1, "failed to load image '%s'",
                image->name);
    }
}

void ZSt13__adjust_heapIPP8GfxImageiS1_PFiS1_S1_EEvT_T0_S6_T1_T2_(
    GfxImage **first, int holeIndex, int len, GfxImage *value, GfxImageCompFunc comp)
{
    int topIndex = holeIndex;
    int secondChild = 2 * holeIndex + 2;

    while (secondChild < len) {
        if (comp(first[secondChild], first[secondChild - 1]))
            secondChild--;
        first[holeIndex] = first[secondChild];
        holeIndex = secondChild;
        secondChild = 2 * secondChild + 2;
    }

    if (secondChild == len) {
        first[holeIndex] = first[len - 1];
        holeIndex = len - 1;
    }

    while (holeIndex > topIndex) {
        int parent = (holeIndex - 1) / 2;
        if (!comp(first[parent], value))
            break;
        first[holeIndex] = first[parent];
        holeIndex = parent;
    }
    first[holeIndex] = value;
}

void ZSt16__insertion_sortIPP8GfxImagePFiS1_S1_EEvT_S5_T0_(
    GfxImage **first, GfxImage **last, GfxImageCompFunc comp)
{
    GfxImage **i;
    if (first == last)
        return;
    for (i = first + 1; i != last; i++) {
        GfxImage *val = *i;
        if (comp(val, *first)) {

            unsigned int n = (unsigned int)((char *)i - (char *)first) & ~3u;
            memmove(first + 1, first, n);
            *first = val;
        } else {

            GfxImage **prev = i - 1;
            GfxImage **hole = i;
            while (comp(val, *prev)) {
                *hole = *prev;
                hole = prev;
                prev--;
            }
            *hole = val;
        }
    }
}

void ZSt16__introsort_loopIPP8GfxImageiPFiS1_S1_EEvT_S5_T0_T1_(
    GfxImage **first, GfxImage **last, int depth_limit, GfxImageCompFunc comp)
{
    while (last - first > 16) {
        if (depth_limit == 0) {

            int n = (int)(last - first);
            int half = (n - 2) / 2;
            int i;
            GfxImage **end;

            for (i = half; i >= 0; i--)
                ZSt13__adjust_heapIPP8GfxImageiS1_PFiS1_S1_EEvT_T0_S6_T1_T2_(
                    first, i, n, first[i], comp);

            for (end = last - 1; end - first > 0; end--) {
                GfxImage *value = *end;
                *end = *first;
                ZSt13__adjust_heapIPP8GfxImageiS1_PFiS1_S1_EEvT_T0_S6_T1_T2_(
                    first, 0, (int)(end - first), value, comp);
            }
            return;
        }

        depth_limit--;

        {
            int n = (int)(last - first);
            int mid = n / 2;
            GfxImage **midPtr = first + mid;
            GfxImage **pivotPtr;
            GfxImage *pivot;
            GfxImage **lo, **hi;

            if (comp(*first, *midPtr)) {
                if (comp(*midPtr, *(last - 1))) {
                    pivotPtr = midPtr;
                } else if (comp(*first, *(last - 1))) {
                    pivotPtr = last - 1;
                } else {
                    pivotPtr = first;
                }
            } else {
                if (comp(*first, *(last - 1))) {
                    pivotPtr = first;
                } else if (comp(*(last - 1), *midPtr)) {
                    pivotPtr = midPtr;
                } else {
                    pivotPtr = last - 1;
                }
            }

            pivot = *pivotPtr;
            lo = first;
            hi = last;

            for (;;) {
                while (!comp(pivot, *lo))
                    lo++;
                hi--;
                while (!comp(*hi, pivot))
                    hi--;
                if (lo >= hi)
                    break;

                {
                    GfxImage *tmp = *lo;
                    *lo = *hi;
                    *hi = tmp;
                }
                lo++;
            }

            ZSt16__introsort_loopIPP8GfxImageiPFiS1_S1_EEvT_S5_T0_T1_(
                lo, last, depth_limit, comp);
            last = lo;
        }
    }
}
