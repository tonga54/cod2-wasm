#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern r_global_permanent_t rgp;

extern void Image_Setup(GfxImage *image, int width, int height, int depth, int semantic, int flags, int imageFormat);
extern int Image_CubemapFace(int face);
extern void Image_UploadData(GfxImage *image, int imageFormat, int face, int mipLevel, byte *pixels);
extern void Image_Create2DTexture(GfxImage *image, int width, int height, int depth, int flags, int format, int unused);
extern float Vec3NormalizeTo(const vec_t *src, vec_t *dst);
extern float Vec3Normalize(vec_t *v);
extern float floorf(float x);
extern float FresnelTerm(float ior0, float ior1, float cosIncident);
extern void AxisTransformVector(const void *matrix, float x, float y, float z, vec_t *out);
extern int Vec3MajorAxis(const vec_t *v);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);
extern void R_LoadJpg(const char *filepath, void **file, byte **pic, int *width, int *height, int *imageFormat);
extern void R_GenerateOutdoorImage(GfxImage *image);
extern void Image_BuildSpecularityMap(float shift, byte *pic);

extern GfxImage *Image_Alloc(const char *name, int category, int semantic, int imageTrack);
static vec3_t lightGridLookupMatrix[3];
static const int faceAxis[6][3];

void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
void Image_Generate3D(GfxImage *image, byte *pixels, int width, int height, int depth, D3DFORMAT imageFormat);
void Image_BuildWaterMap(GfxImage *image);
static void __attribute_regparm__(3) Image_LoadBitmap(GfxImage *image, const GfxImageFileHeader *fileHeader, const byte *data, D3DFORMAT format, int bytesPerPixel);
static void __attribute_regparm__(3) Image_LoadDxtc(GfxImage *image, const GfxImageFileHeader *fileHeader, const byte *data, D3DFORMAT format, int bytesPerBlock);
static void __attribute_regparm__(3) Image_LoadWavelet(GfxImage *image, const byte *fileHeader, const byte *data, D3DFORMAT format, int bytesPerPixel);
void Image_LoadFromData(GfxImage *image, GfxImageFileHeader *fileHeader, const byte *srcData);
static void Image_GetSunHalfAngleForVector(const vec_t *facePos, int ignored, byte *pixel);
static void Image_GetWaterColorForVector(const vec_t *facePos, int packedColor, byte *pixel);
static void Image_GetLightGridWeightsForVector(const vec_t *facePos, int subMap, byte *pixel);
Bool Image_LoadFromFile(GfxImage *image);
GfxImage *R_CreateWaterMap(char *name, int imageWidth, int imageHeight);
typedef void (*CubemapPixelCallback)(const vec_t *facePos, int userData, byte *pixel);
static void Image_GenerateCubemapFunction(GfxImage *image, byte *pic, int res, int userData, CubemapPixelCallback Callback);
Bool Image_LoadRaw(GfxImage *image, const char *filepath, int imageTrack);
static void Image_LoadLightmapWeights(GfxImage *image);
GfxImage *Image_Load(const char *name, int semantic, int imageTrack);

static int Image_ComputeMipCount(int w, int h, int d)
{
    int mips = 1, size = 1;
    while (size < w || size < h || size < d) {
        size <<= 1;
        mips++;
    }
    return mips - 1;
}

void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat)
{
    int face;
    Image_Setup(image, width, height, 1, 3, 0, imageFormat);
    face = Image_CubemapFace(0);
    Image_UploadData(image, imageFormat, face, 0, pixels);
}

void Image_Generate3D(GfxImage *image, byte *pixels, int width, int height, int depth, D3DFORMAT imageFormat)
{
    int face;
    Image_Setup(image, width, height, depth, 0xb, 0, imageFormat);
    face = Image_CubemapFace(0);
    Image_UploadData(image, imageFormat, face, 0, pixels);
}

void Image_BuildWaterMap(GfxImage *image)
{
    int *dxCaps = (int *)r_rendererInUse;
    if (dxCaps[2] == 2) {
        Image_Create2DTexture(image, image->width, image->height, 1, 0x200, 0x16, 0);
    } else {
        Image_Create2DTexture(image, image->width, image->height, 0, 0x200, 0x32, 0);
    }
}

static void __attribute_regparm__(3) Image_LoadBitmap(GfxImage *image, const GfxImageFileHeader *fileHeader, const byte *data, D3DFORMAT format, int bytesPerPixel)
{
    byte *img = (byte *)image;
    int faceCount, mipLevel;
    byte *expandedData = NULL;
    const byte *srcPtr = data;

    Image_Setup(image, fileHeader->dimensions[0], fileHeader->dimensions[1], fileHeader->dimensions[2],
                fileHeader->flags, 0, format);

    faceCount = (image->mapType == 5) ? 6 : 1;

    if (format == 0x16) {
        int pixelCount = image->width * image->height;
        expandedData = (byte *)Hunk_AllocateTempMemoryInternal(pixelCount * 4);
    }

    if (fileHeader->flags & 2) {
        mipLevel = 0;
    } else {
        mipLevel = Image_ComputeMipCount(fileHeader->dimensions[0], fileHeader->dimensions[1], fileHeader->dimensions[2]);
    }

    while (1) {
        if (mipLevel < img[8])
            break;

        {
            int mipW = fileHeader->dimensions[0] >> mipLevel;
            int mipH = fileHeader->dimensions[1] >> mipLevel;
            int face;
            int mipPixels, mipDataSize;

            if (mipW < 1)
                mipW = 1;
            if (mipH < 1)
                mipH = 1;
            mipPixels = mipW * mipH;
            mipDataSize = mipPixels * bytesPerPixel;

            for (face = 0; face < faceCount; face++) {
                int uploadMip = mipLevel - img[8];

                if (format == 0x16) {

                    int p;
                    byte *dst = expandedData;
                    const byte *src = srcPtr;
                    for (p = 0; p < mipPixels; p++) {
                        /* IWI RGB bitmaps contain BGR bytes. The D3D surface
                         * consumes BGRX, including on the WebGL upload path. */
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = 0xFF;
                        dst += 4;
                        src += 3;
                    }
                    Image_UploadData(image, 0x16, Image_CubemapFace(face), uploadMip, expandedData);
                } else {
                    Image_UploadData(image, format, Image_CubemapFace(face), uploadMip, (byte *)srcPtr);
                }
                srcPtr += mipDataSize;
            }
        }
        mipLevel--;
    }

    if (expandedData)
        Hunk_FreeTempMemory(expandedData);
}

static void __attribute_regparm__(3) Image_LoadDxtc(GfxImage *image, const GfxImageFileHeader *fileHeader, const byte *data, D3DFORMAT format, int bytesPerBlock)
{
    byte *img = (byte *)image;
    const byte *hdr = (const byte *)fileHeader;
    int faceCount, mipLevel;
    const byte *srcPtr = data;

    Image_Setup(image, fileHeader->dimensions[0], fileHeader->dimensions[1], fileHeader->dimensions[2],
                fileHeader->flags, 0, format);

    faceCount = (image->mapType == 5) ? 6 : 1;

    if (fileHeader->flags & 2)
        mipLevel = 0;
    else
        mipLevel = Image_ComputeMipCount(fileHeader->dimensions[0], fileHeader->dimensions[1], fileHeader->dimensions[2]);

    while (1) {
        int mipW, mipH, face, mipDataSize;
        int blocksW, blocksH;

        if (mipLevel < img[8])
            break;

        mipW = *(short *)(hdr + 6) >> mipLevel;
        mipH = *(short *)(hdr + 8) >> mipLevel;
        if (mipW < 1)
            mipW = 1;
        if (mipH < 1)
            mipH = 1;

        if (faceCount <= 0) {
            mipLevel--;
            continue;
        }

        blocksW = (mipW + 3) >> 2;
        blocksH = (mipH + 3) >> 2;
        mipDataSize = blocksW * blocksH * bytesPerBlock;

        for (face = 0; face < faceCount; face++) {
            int uploadMip = mipLevel - img[8];
            Image_UploadData(image, format, Image_CubemapFace(face), uploadMip, (byte *)srcPtr);
            srcPtr += mipDataSize;
        }
        mipLevel--;
    }
}

extern void Wavelet_DecompressLevel(const byte *src, byte *dst, void *decode);   /* was int offset -> truncated dst on x64 */
extern int Image_CubemapFace(int faceIndex);
extern void *__Znam(unsigned int size);
extern void __ZdaPv(void *ptr);
extern void Com_Printf(const char *fmt, ...);
static void __attribute_regparm__(3) Image_LoadWavelet(GfxImage *image, const byte *fileHeader,
                                                       const byte *data, D3DFORMAT format, int bytesPerPixel)
{
    int width = *(short *)(fileHeader + 6);
    int height = *(short *)(fileHeader + 8);
    int depth = *(short *)(fileHeader + 10);
    int mipmapCount = *(byte *)(fileHeader + 5);
    int isCubemap = (*(int *)image == 5);
    int faceCount = isCubemap ? 6 : 1;
    int pixelStride = (bytesPerPixel == 3) ? 4 : bytesPerPixel;
    int startLevel;
    int totalSize = width * height * pixelStride;
    byte *hunkBufs[6];
    byte *hunkAddrs[6];
    WaveletDecode decode;
    int i, level, face;

    Image_Setup(image, width, height, depth, mipmapCount, 0, format);

    int picmip = image->picmip.platform[0];

    if (*(byte *)(fileHeader + 5) & 2) {
        startLevel = 0;
    } else {
        int maxDim = 1, levels = 1;
        while (maxDim < width || maxDim < height || maxDim < depth) {
            maxDim *= 2;
            levels++;
        }
        startLevel = levels - 1;
    }

    decode.value = 0;
    decode.bit = 0;
    decode.data = data;
    decode.width = width;
    decode.height = height;
    decode.channels = bytesPerPixel;
    decode.bpp = pixelStride;
    decode.mipLevel = startLevel;
    decode.dataInitialized = 0;

    for (i = 0; i < faceCount; i++) {
        hunkBufs[i] = (byte *)Hunk_AllocateTempMemoryInternal(totalSize);
        hunkAddrs[i] = 0;
    }

    for (level = startLevel; level >= picmip; level--, decode.mipLevel = level) {
        int mipW = width >> level;
        if (mipW < 1)
            mipW = 1;
        int mipH = height >> level;
        if (mipH < 1)
            mipH = 1;
        int sizeForLevel = mipW * mipH * pixelStride;

        if (faceCount <= 0)
            continue;

        int allocSize = (sizeForLevel / 4) * 4;

        for (face = 0; face < faceCount; face++) {

            byte *oldAddr = hunkAddrs[face];
            byte *newAddr = hunkBufs[face] + totalSize - sizeForLevel;
            hunkAddrs[face] = newAddr;

            Wavelet_DecompressLevel((byte *)oldAddr, newAddr, &decode);

            void *pTemp = __Znam(allocSize);
            memcpy(pTemp, newAddr, sizeForLevel);

            {
                unsigned int swapCount = (unsigned int)allocSize / 4;
                unsigned int u;
                for (u = 0; u < swapCount; u++) {
                    DWORD v = ((DWORD *)pTemp)[u];
                    ((DWORD *)pTemp)[u] =
                        (v >> 24) | (v << 24) | ((v << 8) & 0xff0000) | ((v >> 8) & 0xff00);
                }
            }

            int cubeFace = Image_CubemapFace(face);
            Image_UploadData(image, format, cubeFace, level - picmip, (const byte *)pTemp);

            if (pTemp)
                __ZdaPv(pTemp);
        }
    }

    for (i = faceCount - 1; i >= 0; i--)
        Hunk_FreeTempMemory(hunkBufs[i]);

    return;
}

void Image_LoadFromData(GfxImage *image, GfxImageFileHeader *fileHeader, const byte *srcData)
{
    byte *img = (byte *)image;
    const byte *hdr = (const byte *)fileHeader;
    int formatType;

    *(int *)(img + 4) = 0;

    formatType = hdr[4];

    switch (formatType) {
    case 1:
        Image_LoadBitmap(image, fileHeader, srcData, 0x15, 4);
        break;
    case 2:
        Image_LoadBitmap(image, fileHeader, srcData, 0x16, 3);
        break;
    case 3:
        Image_LoadBitmap(image, fileHeader, srcData, 0x33, 2);
        break;
    case 4:
        Image_LoadBitmap(image, fileHeader, srcData, 0x32, 1);
        break;
    case 5:
        Image_LoadBitmap(image, fileHeader, srcData, 0x1c, 1);
        break;
    case 6:
        Image_LoadWavelet(image, (const byte *)fileHeader, srcData, 0x15, 4);
        break;
    case 7:
        Image_LoadWavelet(image, (const byte *)fileHeader, srcData, 0x16, 3);
        break;
    case 8:
        Image_LoadWavelet(image, (const byte *)fileHeader, srcData, 0x33, 2);
        break;
    case 9:
        Image_LoadWavelet(image, (const byte *)fileHeader, srcData, 0x32, 1);
        break;
    case 10:
        Image_LoadWavelet(image, (const byte *)fileHeader, srcData, 0x1c, 1);
        break;
    case 11:
        Image_LoadDxtc(image, fileHeader, srcData, 0x31545844, 8);
        break;
    case 12:
        Image_LoadDxtc(image, fileHeader, srcData, 0x33545844, 16);
        break;
    case 13:
        Image_LoadDxtc(image, fileHeader, srcData, 0x35545844, 16);
        break;
    default:
        break;
    }
}

static void Image_GetSunHalfAngleForVector(const vec_t *facePos, int ignored, byte *pixel)
{
    vec_t dirFromEye[3], halfAngle[3];
    char *drawSurfs;
    const vec_t *sunDir;

    (void)ignored;

    Vec3NormalizeTo(facePos, dirFromEye);

    {
        GfxWorld *world = rgp.world;
        sunDir = world->sunLight.position;
    }

    halfAngle[0] = sunDir[0] - dirFromEye[0];
    halfAngle[1] = sunDir[1] - dirFromEye[1];
    halfAngle[2] = sunDir[2] - dirFromEye[2];
    Vec3Normalize(halfAngle);

    pixel[3] = (byte)(int)floorf((halfAngle[2] * 0.5f + 0.5f) * 255.0f + 0.5f);
    pixel[2] = (byte)(int)floorf((halfAngle[1] * 0.5f + 0.5f) * 255.0f + 0.5f);
    pixel[1] = (byte)(int)floorf((halfAngle[0] * 0.5f + 0.5f) * 255.0f + 0.5f);
    pixel[0] = 0x80;
}

static void Image_GetWaterColorForVector(const vec_t *facePos, int packedColor, byte *pixel)
{
    vec_t dirFromEye[3];
    float fresnel;
    int color;

    Vec3NormalizeTo(facePos, dirFromEye);

    fresnel = FresnelTerm(1.0f, *(float *)&(int){ 0x3faa9fbe }, *(float *)&dirFromEye[1]);

    color = packedColor;

    ((byte *)&color)[0] = (byte)(int)floorf(fresnel * 255.0f + 0.5f);
    *(int *)pixel = color;
}

static inline byte ClampByte(int v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return (byte)v;
}

static inline float SmoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

static void Image_GetLightGridWeightsForVector(const vec_t *facePos, int subMap, byte *pixel)
{
    vec_t transformedPos[3];
    float invMajor;
    int majorAxis;
    float u, v, w;
    float su, sv, sw;
    float w00, w01, w10, w11;

    AxisTransformVector(lightGridLookupMatrix, facePos[0], facePos[1], facePos[2], transformedPos);

    majorAxis = Vec3MajorAxis(transformedPos);

    {
        float majorVal = transformedPos[majorAxis];

        if (majorVal < 0.0f)
            majorVal = -majorVal;
        invMajor = 1.0f / majorVal;
    }
    transformedPos[0] *= invMajor;
    transformedPos[1] *= invMajor;
    transformedPos[2] *= invMajor;

    u = transformedPos[0] * 0.5f + 0.5f;
    v = transformedPos[1] * 0.5f + 0.5f;

    if (subMap == 0)
        w = transformedPos[2] * -0.5f + 0.5f;
    else
        w = transformedPos[2] * 0.5f + 0.5f;

    su = SmoothStep(u);
    sv = SmoothStep(v);
    sw = SmoothStep(w);

    w00 = (1.0f - sv) * sw * (1.0f - su);
    w01 = su * sv * sw;
    w10 = (1.0f - su) * sv * sw;
    w11 = (1.0f - su) * (1.0f - sv) * sw;

    {
        float oneMinusSu = 1.0f - su;
        float oneMinusSv = 1.0f - sv;

        float blend_00 = oneMinusSu * oneMinusSv * sw;
        float blend_10 = su * oneMinusSv * sw;
        float blend_01 = oneMinusSu * sv * sw;
        float blend_11 = su * sv * sw;

        pixel[0] = ClampByte((int)floorf(blend_01 * 255.0f + 0.5f));
        pixel[1] = ClampByte((int)floorf(blend_10 * 255.0f + 0.5f));
        pixel[2] = ClampByte((int)floorf(blend_00 * 255.0f + 0.5f));
        pixel[3] = ClampByte((int)floorf(blend_11 * 255.0f + 0.5f));
    }
}

extern int FS_ReadFile(const char *path, void **buf);
extern void FS_FreeFile(void *buf);
extern void Com_Printf(const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

Bool Image_LoadFromFile(GfxImage *image)
{
    byte *img = (byte *)image;
    char filepath[64];
    void *imageFile;
    const byte *fileData;
    int fileLen;
    int result;

    result = Com_sprintf(filepath, 0x40, "%s%s%s", "images/", image->name, ".iwi");
    if (result < 0) {
        Com_Printf("^1ERROR: filename '%s' too long\n", filepath);
        return 0;
    }

    fileLen = FS_ReadFile(filepath, &imageFile);
    if (fileLen < 0) {
        Com_Printf("^1ERROR: image '%s' is missing\n", filepath);
        return 0;
    }
    if (fileLen == 0) {
        Com_Printf("^1ERROR: image '%s' has 0 length\n", filepath);
        FS_FreeFile(imageFile);
        return 0;
    }

    fileData = (const byte *)imageFile;

    if (fileData[5] & 3)
        img[0xb] = 1;

    if ((*(int *)fileData & 0x00FFFFFF) != 0x695749) {
        Com_Printf("^1ERROR: image '%s' is not an IW image\n", filepath);
        FS_FreeFile(imageFile);
        return 0;
    }

    if (fileData[3] != 5) {
        Com_Printf("^1ERROR: image '%s' is version %i but should be version %i\n",
                   filepath, (int)fileData[3], 5);
        FS_FreeFile(imageFile);
        return 0;
    }

    Image_LoadFromData(image, (GfxImageFileHeader *)fileData, fileData + sizeof(GfxImageFileHeader));
    FS_FreeFile(imageFile);
    return 1;
}

GfxImage *R_CreateWaterMap(char *name, int imageWidth, int imageHeight)
{
    GfxImage *image = Image_Alloc(name, 5, 5, 9);
    image->width = (unsigned short)imageWidth;
    image->height = (unsigned short)imageHeight;

    int *dvar = (int *)r_rendererInUse;
    if (*(int *)(dvar + 2) == 2)
        Image_Create2DTexture(image, image->width, imageHeight, 1, 0x200, 0x16, 0);
    else
        Image_Create2DTexture(image, image->width, imageHeight, 0, 0x200, 0x32, 0);

    return image;
}

static void Image_GenerateCubemapFunction(GfxImage *image, byte *pic, int res, int userData, CubemapPixelCallback Callback)
{
    CubemapPixelCallback cb = Callback;
    float invRes = 1.0f / (float)res;
    int pixelIndex = 0;
    int face;

    for (face = 0; face < 6; face++) {
        const int *axes = &faceAxis[face][0];
        vec_t faceOrigin[3] = { 0, 0, 0 };
        vec_t rightStep[3] = { 0, 0, 0 };
        vec_t upStep[3] = { 0, 0, 0 };
        int t, s;

        {
            int a = axes[0];
            int idx = a >> 1;
            faceOrigin[idx] = (a & 1) ? -1.0f : 1.0f;
        }

        {
            int a = axes[1];
            int idx = a >> 1;
            if (a & 1) {
                faceOrigin[idx] += 1.0f;
                rightStep[idx] = -2.0f;
            } else {
                faceOrigin[idx] += -1.0f;
                rightStep[idx] = 2.0f;
            }
        }

        {
            int a = axes[2];
            int idx = a >> 1;
            if (a & 1) {
                faceOrigin[idx] += 1.0f;
                upStep[idx] = -2.0f;
            } else {
                faceOrigin[idx] += -1.0f;
                upStep[idx] = 2.0f;
            }
        }

        rightStep[0] *= invRes;
        rightStep[1] *= invRes;
        rightStep[2] *= invRes;
        upStep[0] *= invRes;
        upStep[1] *= invRes;
        upStep[2] *= invRes;

        for (t = 0; t < res; t++) {

            vec_t facePos[3];
            facePos[0] = faceOrigin[0] + 0.5f * rightStep[0];
            facePos[1] = faceOrigin[1] + 0.5f * rightStep[1];
            facePos[2] = faceOrigin[2] + 0.5f * rightStep[2];

            {
                float tScale = (float)t + 0.5f;
                facePos[0] += tScale * upStep[0];
                facePos[1] += tScale * upStep[1];
                facePos[2] += tScale * upStep[2];
            }

            for (s = 0; s < res; s++) {
                cb(facePos, userData, pic + pixelIndex * 4);
                pixelIndex++;

                facePos[0] += rightStep[0];
                facePos[1] += rightStep[1];
                facePos[2] += rightStep[2];
            }
        }
    }

    {
        int faceStride = res * res * 4;
        byte *faceData = pic;

        Image_Setup(image, res, res, 1, 7, 0, 0x15);

        for (face = 0; face < 6; face++) {
            Image_UploadData(image, 0x15, Image_CubemapFace(face), 0, faceData);
            faceData += faceStride;
        }
    }
}

Bool Image_LoadRaw(GfxImage *image, const char *filepath, int imageTrack)
{
    byte *pic = NULL;
    void *file;
    int width, height, imageFormat;

    (void)imageTrack;

    R_LoadJpg(filepath, &file, &pic, &width, &height, &imageFormat);

    if (!pic)
        return 0;

    Image_Setup(image, width, height, 1, 3, 0, imageFormat);
    Image_UploadData(image, imageFormat, Image_CubemapFace(0), 0, pic);

    Hunk_FreeTempMemory(pic);
    FS_FreeFile(file);
    return 1;
}

extern float acosf(float x);
extern float floorf(float x);
extern double atan2(double y, double x);
extern float Vec2Normalize(float *v);

static void Image_LoadLightmapWeights(GfxImage *image)
{
    byte pic[32 * 32 * 4];
    int t, s;

    for (t = 0; t < 32; t++) {
        float tv = ((float)t + 0.5f) * (1.0f / 32.0f) * 2.0f - 1.0f;

        for (s = 0; s < 32; s++) {
            float sv = ((float)s + 0.5f) * (1.0f / 32.0f) * 2.0f - 1.0f;

            float zSq = 1.0f - sv * sv - tv * tv;
            float dir[2];
            dir[0] = sv;

            if (zSq < 0.0f) {

                Vec2Normalize(dir);
                zSq = 0.0f;
            }

            float z = __builtin_sqrtf(zSq);

            float angle = (float)(atan2((double)sv, (double)tv) * 0.477464829275686 - 0.75);

            if (angle < 0.0f)
                angle += 3.0f;
            if (angle > 3.0f)
                angle -= 3.0f;

            float w0, w1, w2;

            if (angle < 1.0f) {

                w2 = 1.0f - angle;
                w1 = 0.0f;
                w0 = 0.0f;
            } else if (angle < 2.0f) {

                w1 = angle - 1.0f;
                w0 = 1.0f - w1;
                w2 = 0.0f;
            } else {

                w2 = angle - 2.0f;
                w0 = 1.0f - w2;
                w1 = 0.0f;
            }

            float falloff = acosf(z) / -0.9553166031837463f + 1.0f;
            float complement, mainWeight;

            if (falloff < 0.0f) {

                mainWeight = 1.0f;
                falloff = 0.0f;
            } else if (falloff > 1.0f) {

                complement = 0.0f;
                mainWeight = 255.0f * falloff;
                goto write_pixel;
            } else {
                complement = 1.0f - falloff;
                mainWeight = 255.0f * falloff;
            }

        write_pixel:;
            byte *pixel = &pic[(t * 32 + s) * 4];
            byte r = (byte)(int)floorf(mainWeight + 0.5f);
            byte g = (byte)(int)floorf(w2 * complement * 255.0f + 0.5f);
            byte b = (byte)(int)floorf(w1 * complement * 255.0f + 0.5f);
            byte a = (byte)(int)floorf(complement * w0 * 255.0f + 0.5f);
            pixel[0] = b;
            pixel[1] = g;
            pixel[2] = r;
            pixel[3] = a;
        }
    }

    Image_Setup(image, 32, 32, 1, 3, 0, 0x15);
    int face = Image_CubemapFace(0);
    Image_UploadData(image, 0x15, face, 0, pic);
}

static GfxImage *Image_CreateSolidColor(const char *name, int semantic, int imageTrack,
                                        byte r, byte g, byte b, byte a)
{
    byte pic[4];
    GfxImage *image = Image_Alloc(name, 1, semantic, imageTrack);
    pic[0] = b;
    pic[1] = g;
    pic[2] = r;
    pic[3] = a;
    Image_Setup(image, 1, 1, 1, 3, 0, 0x15);
    Image_UploadData(image, 0x15, Image_CubemapFace(0), 0, pic);
    return image;
}

GfxImage *Image_Load(const char *name, int semantic, int imageTrack)
{
    GfxImage *image;

    if (name[0] != '$') {
        image = Image_Alloc(name, 3, (byte)semantic, imageTrack);
        if (!Image_LoadFromFile(image))
            return NULL;
        return image;
    }

    if (!memcmp(name, "$white", 7)) {
        return Image_CreateSolidColor(name, (byte)semantic, imageTrack, 0xFF, 0xFF, 0xFF, 0xFF);
    }
    if (!memcmp(name, "$black", 7)) {
        return Image_CreateSolidColor(name, (byte)semantic, imageTrack, 0x00, 0x00, 0x00, 0xFF);
    }
    if (!memcmp(name, "$identitynormalmap", 19)) {

        return Image_CreateSolidColor(name, (byte)semantic, imageTrack, 0x80, 0x80, 0xFF, 0x80);
    }
    if (!memcmp(name, "$specularity", 13)) {
        byte pic[0x2000];
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        Image_BuildSpecularityMap(0, pic);
        Image_Setup(image, 0x20, 0x100, 1, 3, 0, 0x32);
        Image_UploadData(image, 0x32, Image_CubemapFace(0), 0, pic);
        return image;
    }
    if (!memcmp(name, "$outdoor", 9)) {
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        R_GenerateOutdoorImage(image);
        return image;
    }
    if (!memcmp(name, "$lightmapweights", 17)) {
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        Image_LoadLightmapWeights(image);
        return image;
    }
    if (!memcmp(name, "$lightgridweights0", 19)) {
        byte pic[0x6000];
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        Image_GenerateCubemapFunction(image, pic, 0x20, 0, (CubemapPixelCallback)Image_GetLightGridWeightsForVector);
        return image;
    }
    if (!memcmp(name, "$lightgridweights1", 19)) {
        byte pic[0x6000];
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        Image_GenerateCubemapFunction(image, pic, 0x20, 1, (CubemapPixelCallback)Image_GetLightGridWeightsForVector);
        return image;
    }
    if (!memcmp(name, "$watercolor", 12)) {
        byte pic[0x1800];
        int waterColor;
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);

        waterColor = 0;
        ((byte *)&waterColor)[0] = 0xFF;
        ((byte *)&waterColor)[1] = 0x33;
        ((byte *)&waterColor)[2] = 0x40;
        waterColor = (waterColor & 0x00FFFFFF) | 0x4D000000;
        Image_GenerateCubemapFunction(image, pic, 0x10, waterColor,
                                      (CubemapPixelCallback)Image_GetWaterColorForVector);
        return image;
    }
    if (!memcmp(name, "$sunhalfangle", 14)) {
        byte *pic;
        image = Image_Alloc(name, 1, (byte)semantic, imageTrack);
        pic = (byte *)Hunk_AllocateTempMemoryInternal(0x18000);
        Image_GenerateCubemapFunction(image, pic, 0x40, 0,
                                      (CubemapPixelCallback)Image_GetSunHalfAngleForVector);
        Hunk_FreeTempMemory(pic);
        return image;
    }

    Com_Printf("ERROR: Unknown built-in image '%s'", name);
    return NULL;
}
