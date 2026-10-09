#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

extern void I_strncpyz(char *dest, const char *src, int destsize);

static int lastNumber;
static vec3_t cubemapShotAxis[7][3];
static int cubeShotGlob;
static int pendingScreenshotType = -1;
static char pendingScreenshotFilename[256];
static int pendingScreenshotSilent;
static int pendingScreenshotDelayFrames;

extern DxGlobals *dx;
extern int alwaysfails;
extern refimport_t ri;
extern vidConfig_t vidConfig;
extern r_global_permanent_t rgp;
extern int **r_gammaSetting;

void R_BeginCubemapShot(const int pixelWidthHeight, const int pixelBorder);
static Bool R_GetFrontBufferData(int width, int height, int bytesPerPixel, byte *buffer);
void R_EndCubemapShot(const CubemapShot shotIndex);
static void R_UpsamplePixelData(int oldSize, int newSize, int stride, int bytesPerPixel, byte *src, byte *dst);
static void R_DownsamplePixelData(int oldSize, int newSize, int stride, int bytesPerPixel, byte *src, byte *dst);
static byte *R_TakeResampledScreenshot(int width, int height, int bytesPerPixel, int headerSize);
void R_LevelShot(void);
void R_SaveCubemapShot(const char *filename, const CubemapShot shotIndex, const float n0, const float n1);
void R_LightingFromCubemapShots(const vec_t *baseColor);
void R_ScreenshotCommand(GfxScreenshotType type);
void R_CapturePendingScreenshotBeforePresent(void);
int R_HasPendingScreenshot(void);

extern float floorf(float x);
extern double pow(double base, double exp);

extern void RB_SetRenderTarget(int target);
extern void R_GammaCorrect(byte *buffer, int bufferSizeInBytes);
extern void R_SaveJpg(const char *filename, int quality, int width, int height, byte *buffer);
extern int Com_sprintf(char *dest, int size, const char *format, ...);
extern float Vec3Normalize(vec3_t v);
extern float FresnelTerm(float n0, float n1, float cosAngle);
extern int MacDisplay_GetCardType(void);
extern int MacOpenGLUtils_GetPCPixelShaderVersion(void);
extern short unsigned int MacDisplay_GetCurrentDimensions(int *width, int *height);

static void R_ClearGLErrors(void)
{
    while (glGetError() != 0) {
    }
}

static Bool R_ReadbackIsAllBlack(const byte *buffer, int width, int height)
{
    int pixelCount = width * height;
    int i;

    for (i = 0; i < pixelCount; i++) {
        const byte *p = buffer + i * 4;
        if (p[0] || p[1] || p[2])
            return 0;
    }

    return 1;
}

void R_BeginCubemapShot(const int pixelWidthHeight, const int pixelBorder)
{
    void *device;
    int *vtable;

    dx->cubemapShotRes = (short unsigned int)pixelWidthHeight;
    dx->cubemapShotPixelBorder = (short unsigned int)pixelBorder;

    RB_SetRenderTarget(0);

    do {
        device = (void *)dx->device;
        vtable = *(int **)device;

        ((void(D3DVTCC *)(void *, int, int, int, int, float, int))vtable[0xAC / 4])(device, 0, 0, 7, 0xFFFF00FF, 1.0f, 0);
    } while (*(volatile int *)&alwaysfails != 0);
}

static Bool R_GetFrontBufferData(int width, int height, int bytesPerPixel, byte *buffer)
{
#ifdef __EMSCRIPTEN__
    int viewport[4], packAlignment;
    if (!buffer || width <= 0 || height <= 0 ||
        (bytesPerPixel != 3 && bytesPerPixel != 4)) return 0;
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (width > viewport[2] || height > viewport[3]) return 0;
    byte *pixels = malloc((size_t)width * height * 4);
    if (!pixels) return 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    R_ClearGLErrors();
    /* WebGL has one default framebuffer; GL_FRONT is not a valid read target. */
    glReadPixels(0, viewport[3] - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    unsigned int error = glGetError();
    glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
    if (!error) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                memcpy(buffer + ((height - y - 1) * width + x) * bytesPerPixel,
                       pixels + (y * width + x) * 4, bytesPerPixel);
            }
        }
    }
    free(pixels);
    return error == GL_NO_ERROR;
#else

    int rowBytes;
    byte *tempBuffer;
    int NumTexUnits;
    int ScreenWidth, ScreenHeight;
    int cardType;
    int i;
    int row, col;
    byte *srcPixel;
    byte *dstPixel;
    unsigned int readErr;
    int viewport[4] = { 0, 0, 0, 0 };
    int readBuffer = 0;
    int drawBuffer = 0;
    byte centerBack[4] = { 0, 0, 0, 0 };
    byte centerFront[4] = { 0, 0, 0, 0 };

    rowBytes = width * 4;
    tempBuffer = (byte *)calloc(1, rowBytes * height);
    if (!tempBuffer) {
        return 0;
    }

    R_ClearGLErrors();

    glPushAttrib(0xFFFFF);
    glPushClientAttrib(0xFFFFFFFF);

    glDisable(0x8620);

    cardType = MacDisplay_GetCardType();
    if (cardType == 2) {
        glDisable(0x86DE);
        glDisable(0x8522);
    } else {
        cardType = MacDisplay_GetCardType();
        if (cardType == 1 + 1) {

        }
        if (cardType == 2) {
            glDisable(0x8200);
        }
    }

    if (MacOpenGLUtils_GetPCPixelShaderVersion() > 0xFFFF01FF) {
        glDisable(0x8804);
    }
    glDisable(0x0B50);
    glDisable(0x0BE2);
    glDisable(0x0BC0);
    glDisable(0x0B71);
    glDisable(0x0BD0);
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    glDisable(0x0B44);
    glDisable(0x0B60);
    glActiveTextureARB(0x84C0);
    glDisable(0x0B90);

    glGetIntegerv(0x84E2, &NumTexUnits);
    for (i = 0; i < NumTexUnits; i++) {
        glActiveTextureARB(0x84C0 + i);
        glDisable(0x0DE1);
        glDisable(0x84F5);
        glDisable(0x806F);
        glDisable(0x8513);
    }

    MacDisplay_GetCurrentDimensions(&ScreenWidth, &ScreenHeight);

    R_ClearGLErrors();
    glGetIntegerv(0x0BA2, viewport);
    glGetIntegerv(0x0C02, &readBuffer);
    glGetIntegerv(0x0C01, &drawBuffer);

    if (ScreenWidth < width || ScreenHeight < height) {
        if (viewport[2] >= width && viewport[3] >= height) {
            ScreenWidth = viewport[2];
            ScreenHeight = viewport[3];
        } else {
            ScreenWidth = width;
            ScreenHeight = height;
        }
    }

    glReadBuffer(0x0405);
    glReadPixels(width / 2, height / 2, 1, 1, 0x1908, 0x1401, centerBack);
    glReadBuffer(0x0404);
    glReadPixels(width / 2, height / 2, 1, 1, 0x1908, 0x1401, centerFront);
    ri.Printf(0, "[screenshot-trace] readback pre screen=%dx%d request=%dx%d viewport=(%d,%d,%d,%d) rb=0x%x db=0x%x centerBack=(%u,%u,%u,%u) centerFront=(%u,%u,%u,%u)\n",
              ScreenWidth, ScreenHeight, width, height,
              viewport[0], viewport[1], viewport[2], viewport[3],
              readBuffer, drawBuffer,
              centerBack[0], centerBack[1], centerBack[2], centerBack[3],
              centerFront[0], centerFront[1], centerFront[2], centerFront[3]);

    glReadBuffer(0x0405);
    glReadPixels(0, ScreenHeight - height, width, height, 0x1908, 0x1401, tempBuffer);
    readErr = glGetError();

    if (readErr != 0) {
        R_ClearGLErrors();
        glReadBuffer(0x0404);
        glReadPixels(0, ScreenHeight - height, width, height, 0x1908, 0x1401, tempBuffer);
        readErr = glGetError();
    } else if (R_ReadbackIsAllBlack(tempBuffer, width, height)) {
        unsigned int altErr;
        R_ClearGLErrors();
        glReadBuffer(0x0404);
        glReadPixels(0, ScreenHeight - height, width, height, 0x1908, 0x1401, tempBuffer);
        altErr = glGetError();
        if (altErr != 0) {
            readErr = altErr;
        } else if (R_ReadbackIsAllBlack(tempBuffer, width, height)) {
            ri.Printf(0, "R_GetFrontBufferData: both GL_BACK and GL_FRONT read back black\n");
        }
    }

    glPopClientAttrib();
    glPopAttrib();

    if (readErr != 0) {
        ri.Printf(0, "R_GetFrontBufferData: glReadPixels failed err=0x%x screen=%dx%d request=%dx%d bpp=%d\n",
                  readErr, ScreenWidth, ScreenHeight, width, height, bytesPerPixel);
        free(tempBuffer);
        return 0;
    }

    if (bytesPerPixel == 3) {
        int offset;
        offset = height * 3 - 3;
        for (row = 0; row < height; row++) {
            srcPixel = tempBuffer + rowBytes * row;
            dstPixel = buffer + offset * width;
            for (col = 0; col < width; col++) {
                dstPixel[0] = srcPixel[0];
                dstPixel[1] = srcPixel[1];
                dstPixel[2] = srcPixel[2];
                dstPixel += 3;
                srcPixel += 4;
            }
            offset -= 3;
        }
        free(tempBuffer);
        return 1;
    } else if (bytesPerPixel == 4) {
        int offset;
        offset = (height - 1) * 4;
        for (row = 0; row < height; row++) {
            srcPixel = tempBuffer + rowBytes * row;
            dstPixel = buffer + offset * width;
            for (col = 0; col < width; col++) {
                dstPixel[0] = srcPixel[0];
                dstPixel[1] = srcPixel[1];
                dstPixel[2] = srcPixel[2];
                dstPixel[3] = srcPixel[3];
                dstPixel += 4;
                srcPixel += 4;
            }
            offset -= 4;
        }
        free(tempBuffer);
        return 1;
    }

    free(tempBuffer);
    return 0;
#endif
}

void R_EndCubemapShot(const CubemapShot shotIndex)
{
    int bufferSizeInBytes;
    byte *buffer;
    int res;
    int rowIndex, colIndex;
    int swapBuffer;
    int *pixels;
    int gammaVal;

    res = dx->cubemapShotRes;
    bufferSizeInBytes = res * res * 4;
    buffer = (byte *)ri.Z_MallocInternal(bufferSizeInBytes);

    ((byte **)&cubeShotGlob)[shotIndex - 1] = buffer;

    R_GetFrontBufferData(dx->cubemapShotRes, dx->cubemapShotPixelBorder, 4, buffer);

    pixels = (int *)buffer;
    for (rowIndex = 0; rowIndex < dx->cubemapShotRes / 2; rowIndex++) {
        res = dx->cubemapShotRes;
        for (colIndex = 0; colIndex < res; colIndex++) {
            int temp;
            temp = pixels[rowIndex * res + colIndex];
            pixels[rowIndex * res + colIndex] = pixels[(res - rowIndex - 1) * res + colIndex];
            pixels[(res - rowIndex - 1) * res + colIndex] = temp;
        }
    }

    gammaVal = (*r_gammaSetting)[2];
    if (gammaVal > 0 && vidConfig.deviceSupportsGamma) {
        R_GammaCorrect(buffer, bufferSizeInBytes);
    }

    RB_SetRenderTarget(0);
}

static void R_UpsamplePixelData(int oldSize, int newSize, int stride, int bytesPerPixel, byte *src, byte *dst)
{
    int nextSample;
    byte *currSrc;
    int backwardWeight;
    int forwardWeight;
    float colorScale;
    int i;

    nextSample = stride * bytesPerPixel;
    currSrc = src + (oldSize - 1) * nextSample;

    dst = dst + (newSize - 1) * nextSample;
    dst[nextSample] = currSrc[0];
    dst[nextSample + 1] = currSrc[1];
    dst[nextSample + 2] = currSrc[2];

    forwardWeight = newSize - oldSize;
    backwardWeight = newSize + oldSize;
    colorScale = 0.5f / (float)newSize;

    for (i = 0; i < newSize; i++) {
        if (currSrc >= src) {

            int val0, val1, val2;
            val0 = (int)currSrc[0] * backwardWeight + (int)currSrc[nextSample] * forwardWeight;
            dst[0] = (byte)(int)floorf((float)val0 * colorScale + 0.5f);

            val1 = (int)currSrc[1] * backwardWeight + (int)currSrc[nextSample + 1] * forwardWeight;
            dst[1] = (byte)(int)floorf((float)val1 * colorScale + 0.5f);

            val2 = (int)currSrc[2] * backwardWeight + (int)currSrc[nextSample + 2] * forwardWeight;
            dst[2] = (byte)(int)floorf((float)val2 * colorScale + 0.5f);
        } else {

            int totalWeight = backwardWeight + forwardWeight;
            dst[0] = (byte)(int)floorf((float)((int)src[0] * totalWeight) * colorScale + 0.5f);
            dst[1] = (byte)(int)floorf((float)((int)src[1] * totalWeight) * colorScale + 0.5f);
            dst[2] = (byte)(int)floorf((float)((int)src[2] * totalWeight) * colorScale + 0.5f);
        }

        dst -= nextSample;
        backwardWeight += oldSize * 2;
        forwardWeight -= oldSize * 2;

        if (forwardWeight < 0) {
            backwardWeight -= newSize * 2;
            forwardWeight += newSize * 2;
            currSrc -= nextSample;
        }
    }
}

static void R_DownsamplePixelData(int oldSize, int newSize, int stride, int bytesPerPixel, byte *src, byte *dst)
{
    float colorScale;
    int nextSample;
    int residual;
    int column;
    int accR, accG, accB;

    colorScale = 1.0f / (float)oldSize;
    nextSample = stride * bytesPerPixel;

    residual = newSize;
    for (column = 0; column < newSize; column++) {
        accR = (int)src[0] * residual;
        accG = (int)src[1] * residual;
        accB = (int)src[2] * residual;
        src += nextSample;

        residual = residual + newSize - oldSize;

        while (residual <= 0) {
            accR += (int)src[0] * newSize;
            accG += (int)src[1] * newSize;
            accB += (int)src[2] * newSize;
            src += nextSample;
            residual += newSize;
        }

        {
            int frac = newSize - residual;
            accG += (int)src[1] * frac;
            accB += (int)src[2] * frac;
            accR += (int)src[0] * frac;
        }

        dst[0] = (byte)(int)floorf((float)accR * colorScale + 0.5f);
        dst[1] = (byte)(int)floorf((float)accG * colorScale + 0.5f);
        dst[2] = (byte)(int)floorf((float)accB * colorScale + 0.5f);
        dst += nextSample;
    }
}

static byte *R_TakeResampledScreenshot(int width, int height, int bytesPerPixel, int headerSize)
{
    int oldWidth, oldHeight;
    int allocSize;
    byte *buffer;
    byte *pixelData;
    int srcRowStride, dstRowStride;
    byte *srcRow, *dstRow;
    int i;
    int maxWidth, maxHeight;

    maxWidth = vidConfig.width;
    if (width < maxWidth) {
        maxWidth = width;
    }
    maxHeight = vidConfig.height;
    if (height < maxHeight) {
        maxHeight = height;
    }

    allocSize = maxWidth * maxHeight * bytesPerPixel + headerSize;
    buffer = (byte *)ri.Z_MallocInternal(allocSize);

    if (!R_GetFrontBufferData(vidConfig.width, vidConfig.height, bytesPerPixel, buffer + headerSize)) {
        ri.Z_FreeInternal(buffer);
        buffer = NULL;
        return buffer;
    }

    pixelData = buffer + headerSize;
    oldHeight = vidConfig.height;
    oldWidth = vidConfig.width;

    if (width < oldWidth) {

        if (oldHeight > 0) {
            srcRowStride = oldWidth * bytesPerPixel;
            dstRowStride = width * bytesPerPixel;
            srcRow = pixelData;
            dstRow = pixelData;
            for (i = 0; i < oldHeight; i++) {
                R_DownsamplePixelData(oldWidth, width, 1, bytesPerPixel, srcRow, dstRow);
                srcRow += srcRowStride;
                dstRow += dstRowStride;
            }
        }
    } else if (width > oldWidth) {

        srcRowStride = oldWidth * bytesPerPixel;
        dstRowStride = width * bytesPerPixel;
        srcRow = pixelData + (oldHeight - 1) * srcRowStride;
        dstRow = pixelData + (oldHeight - 1) * dstRowStride;
        if (oldHeight > 0) {
            for (i = 0; i < oldHeight; i++) {
                R_UpsamplePixelData(oldWidth, width, 1, bytesPerPixel, srcRow, dstRow);
                srcRow -= srcRowStride;
                dstRow -= dstRowStride;
            }
        }
    }

    if (height < oldHeight) {

        if (width > 0) {
            srcRow = pixelData;
            dstRow = pixelData;
            for (i = 0; i < width; i++) {
                R_DownsamplePixelData(oldHeight, height, width, bytesPerPixel, srcRow, dstRow);
                srcRow += bytesPerPixel;
                dstRow += bytesPerPixel;
            }
        }
    } else if (height > oldHeight) {

        dstRow = pixelData + (width - 1) * bytesPerPixel;
        if (width > 0) {
            srcRow = dstRow;
            for (i = 0; i < width; i++) {
                R_UpsamplePixelData(oldHeight, height, width, bytesPerPixel, srcRow, dstRow);
                dstRow -= bytesPerPixel;
                srcRow -= bytesPerPixel;
            }
        }
    }

    return buffer;
}

void R_LevelShot(void)
{
    char checkname[256];
    byte *buffer;

    if (rgp.world) {
        sprintf(checkname, "levelshots/%s.tga", rgp.world->baseName);
    } else {
        sprintf(checkname, "levelshots/%s.tga", "screenshot");
    }

    buffer = R_TakeResampledScreenshot(128, 128, 3, 18);
    if (buffer) {

        *(int *)(buffer + 0) = 0;
        *(int *)(buffer + 4) = 0;
        *(int *)(buffer + 8) = 0;
        *(int *)(buffer + 12) = 0;
        *(short *)(buffer + 16) = 0;
        buffer[2] = 2;
        buffer[12] = 128;
        buffer[14] = 128;
        buffer[16] = 24;

        ri.FS_WriteFile(checkname, buffer, 0xC012);
        ri.Z_FreeInternal(buffer);
        ri.Printf(0, "Wrote %s\n", checkname);
    }
}

void R_SaveCubemapShot(const char *filename, const CubemapShot shotIndex, const float n0, const float n1)
{
    int imgIndex;
    int fileSize;
    byte *targa;
    byte *srcBuffer;
    int res;
    int rowIndex, colIndex;
    vec3_t dir;
    float scale;

    imgIndex = shotIndex - 1;

    res = dx->cubemapShotRes;
    fileSize = res * res * 4 + 18;
    targa = (byte *)ri.Z_MallocInternal(fileSize);

    res = dx->cubemapShotRes;

    *(int *)(targa + 0) = 0;
    *(int *)(targa + 4) = 0;
    *(int *)(targa + 8) = 0;
    *(int *)(targa + 12) = 0;
    *(short *)(targa + 16) = 0;
    targa[2] = 2;
    targa[12] = (byte)res;
    targa[13] = (byte)(res >> 8);
    targa[14] = (byte)res;
    targa[15] = (byte)(res >> 8);
    targa[16] = 32;

    srcBuffer = ((byte **)&cubeShotGlob)[imgIndex];

    res = dx->cubemapShotRes;
    if (res > 0) {
        for (rowIndex = 0; rowIndex < res; rowIndex++) {
            if (res > 0) {
                for (colIndex = 0; colIndex < res; colIndex++) {
                    int srcOffset = (rowIndex * res + colIndex) * 4;
                    byte *dst = targa + 18 + srcOffset;
                    dst[2] = srcBuffer[srcOffset + 2];
                    dst[1] = srcBuffer[srcOffset + 1];
                    dst[0] = srcBuffer[srcOffset + 0];
                    dst[3] = srcBuffer[srcOffset + 3];
                }
            }
        }
    }

    res = dx->cubemapShotRes;
    if (res > 0) {
        for (rowIndex = 0; rowIndex < res; rowIndex++) {
            if (res > 0) {
                for (colIndex = 0; colIndex < res; colIndex++) {
                    float fresnelVal;
                    int axisBase;
                    vec3_t *faceAxis;

                    scale = (float)res;
                    axisBase = shotIndex * 9;
                    faceAxis = &cubemapShotAxis[0][0];

                    dir[0] = ((float *)faceAxis)[axisBase + 0] * scale * 0.5f;
                    dir[1] = ((float *)faceAxis)[axisBase + 1] * scale * 0.5f;
                    dir[2] = ((float *)faceAxis)[axisBase + 2] * scale * 0.5f;

                    {
                        float u = (float)colIndex - scale * 0.5f + 0.5f;
                        dir[0] += ((float *)faceAxis)[axisBase + 3] * u;
                        dir[1] += ((float *)faceAxis)[axisBase + 4] * u;
                        dir[2] += ((float *)faceAxis)[axisBase + 5] * u;
                    }

                    {
                        float v = (float)rowIndex - scale * 0.5f + 0.5f;
                        dir[0] += ((float *)faceAxis)[axisBase + 6] * v;
                        dir[1] += ((float *)faceAxis)[axisBase + 7] * v;
                        dir[2] += ((float *)faceAxis)[axisBase + 8] * v;
                    }

                    Vec3Normalize(dir);

                    fresnelVal = FresnelTerm(n0, n1, dir[2]);
                    targa[18 + (rowIndex * res + colIndex) * 4 + 3] = (byte)(int)floorf(fresnelVal * 255.0f + 0.5f);
                }
            }
        }
    }

    ri.FS_WriteFile(filename, targa, fileSize);
    ri.Z_FreeInternal(targa);
    ri.Z_FreeInternal(((byte **)&cubeShotGlob)[imgIndex]);
}

void R_LightingFromCubemapShots(const vec_t *baseColor)
{
    int pixelsPerFace;
    float *linearColors;
    float *faceColors[6];
    int height;
    int faceIndex;
    int x, y;
    int pixelIndex;
    vec3_t dir;
    vec3_t sourceDir;
    int sampleCount;
    float sampleR, sampleG, sampleB;
    float invSampleCount;
    float ambientR, ambientG, ambientB;
    byte *pixel;
    float heightF;

    pixelsPerFace = (int)dx->cubemapShotRes * (int)dx->cubemapShotRes;
    linearColors = (float *)ri.Z_MallocInternal(pixelsPerFace * 9 * 8);

    faceColors[0] = linearColors + pixelsPerFace * 3;
    faceColors[1] = faceColors[0] + pixelsPerFace * 3;
    faceColors[2] = faceColors[1] + pixelsPerFace * 3;
    faceColors[3] = faceColors[2] + pixelsPerFace * 3;
    faceColors[4] = faceColors[3] + pixelsPerFace * 3;

    height = dx->cubemapShotRes;

    {
        int faceIdx;
        byte **cubeShotPtr = (byte **)&cubeShotGlob;
        for (faceIdx = 1; faceIdx <= 6; faceIdx++) {
            float *faceLinear;
            if (faceIdx == 1) {
                faceLinear = linearColors;
            } else {
                faceLinear = faceColors[faceIdx - 2];
            }

            if (height > 0) {
                pixelIndex = 0;
                for (y = 0; y < height; y++) {
                    for (x = 0; x < height; x++) {
                        byte *srcPixel = cubeShotPtr[faceIdx - 1] + pixelIndex * 4;
                        faceLinear[pixelIndex * 3 + 0] = (float)pow((double)((float)srcPixel[2] * (1.0f / 255.0f)), 2.2);
                        faceLinear[pixelIndex * 3 + 1] = (float)pow((double)((float)srcPixel[1] * (1.0f / 255.0f)), 2.2);
                        faceLinear[pixelIndex * 3 + 2] = (float)pow((double)((float)srcPixel[0] * (1.0f / 255.0f)), 2.2);
                        pixelIndex++;
                    }
                }
            }
        }
    }

    height = dx->cubemapShotRes;
    heightF = (float)height;

    for (faceIndex = 0; faceIndex <= 5; faceIndex++) {
        float *faceLinear;
        byte **cubeShotPtr = (byte **)&cubeShotGlob;

        if (height <= 0) {
            continue;
        }

        for (y = 0; y < height; y++) {
            float yNorm = ((float)y + 0.5f) * 2.0f / heightF - 1.0f;

            for (x = 0; x < height; x++) {
                float xNorm = ((float)x + 0.5f) * 2.0f / heightF - 1.0f;
                int axisBase;
                float *axis;
                int sampleFace;

                axisBase = (faceIndex + 1) * 9;
                axis = (float *)cubemapShotAxis;

                dir[0] = axis[axisBase + 0] + axis[axisBase + 3] * xNorm;
                dir[1] = axis[axisBase + 1] + axis[axisBase + 4] * xNorm;
                dir[2] = axis[axisBase + 2] + axis[axisBase + 5] * xNorm;

                dir[0] += axis[axisBase + 6] * yNorm;
                dir[1] += axis[axisBase + 7] * yNorm;
                dir[2] += axis[axisBase + 8] * yNorm;

                Vec3Normalize(dir);

                pixel = cubeShotPtr[faceIndex] + (y * height + x) * 4;
                sampleCount = 0;
                sampleR = 0.0f;
                sampleG = 0.0f;
                sampleB = 0.0f;

                for (sampleFace = 1; sampleFace <= 6; sampleFace++) {
                    int sy, sx;
                    float *sampleColors;

                    if (sampleFace == 1) {
                        sampleColors = linearColors;
                    } else {
                        sampleColors = faceColors[sampleFace - 2];
                    }

                    for (sy = 0; sy < height; sy++) {
                        float syNorm = ((float)sy + 0.5f) * 2.0f / heightF - 1.0f;
                        int sampleRow = sy * height;

                        for (sx = 0; sx < height; sx++) {
                            float sxNorm = ((float)sx + 0.5f) * 2.0f / heightF - 1.0f;
                            int sAxBase = sampleFace * 9;
                            float dot;

                            sourceDir[0] = axis[sAxBase + 0] + axis[sAxBase + 3] * sxNorm;
                            sourceDir[1] = axis[sAxBase + 1] + axis[sAxBase + 4] * sxNorm;
                            sourceDir[2] = axis[sAxBase + 2] + axis[sAxBase + 5] * sxNorm;

                            sourceDir[0] += axis[sAxBase + 6] * syNorm;
                            sourceDir[1] += axis[sAxBase + 7] * syNorm;
                            sourceDir[2] += axis[sAxBase + 8] * syNorm;

                            Vec3Normalize(sourceDir);

                            dot = dir[0] * sourceDir[0] + dir[1] * sourceDir[1] + dir[2] * sourceDir[2];

                            if (dot > 0.0f) {
                                float *samplePixel = sampleColors + (sampleRow + sx) * 3;
                                sampleR += dot * samplePixel[0];
                                sampleG += dot * samplePixel[1];
                                sampleB += dot * samplePixel[2];
                                sampleCount++;
                            }
                        }
                    }
                }

                invSampleCount = 1.0f / (float)sampleCount;
                ambientR = (float)pow((double)(sampleR * invSampleCount), 0.454545);
                ambientG = (float)pow((double)(sampleG * invSampleCount), 0.454545);
                ambientB = (float)pow((double)(sampleB * invSampleCount), 0.454545);

                ambientR *= baseColor[0];
                ambientG *= baseColor[1];
                ambientB *= baseColor[2];

                {
                    float rVal, gVal, bVal;

                    if (ambientR >= 1.0f) {
                        rVal = 255.0f;
                    } else {
                        rVal = ambientR * 255.0f;
                    }
                    pixel[2] = (byte)(int)floorf(rVal + 0.5f);

                    if (ambientG >= 1.0f) {
                        gVal = 255.0f;
                    } else {
                        gVal = ambientG * 255.0f;
                    }
                    pixel[1] = (byte)(int)floorf(gVal + 0.5f);

                    if (ambientB >= 1.0f) {
                        bVal = 255.0f;
                    } else {
                        bVal = ambientB * 255.0f;
                    }
                    pixel[0] = (byte)(int)floorf(bVal + 0.5f);
                    pixel[3] = 0xFF;
                }
            }
        }
    }

    ri.Z_FreeInternal(linearColors);
}

static void R_CaptureScreenshotNow(GfxScreenshotType type, const char *filename, int silent)
{
    byte *buffer;
    int width;
    int height;
    int fileSize;

    if (type == 0) {
        width = vidConfig.width;
        height = vidConfig.height;

        buffer = (byte *)ri.Z_MallocInternal(width * height * 3);
        ri.Printf(0, "[screenshot-trace] jpg filename='%s' size=%dx%d buffer=%p\n",
                  filename, width, height, buffer);
        if (R_GetFrontBufferData(width, height, 3, buffer)) {
            ri.Printf(0, "[screenshot-trace] readback ok, saving '%s'\n", filename);
            R_SaveJpg(filename, 90, width, height, buffer);
            ri.Printf(0, "[screenshot-trace] save requested '%s'\n", filename);
        } else {
            ri.Printf(0, "[screenshot-trace] readback failed '%s'\n", filename);
        }
        ri.Z_FreeInternal(buffer);
    } else if (type == 1) {
        width = vidConfig.width;
        height = vidConfig.height;

        fileSize = width * height * 3 + 18;
        buffer = (byte *)ri.Z_MallocInternal(fileSize);

        *(int *)(buffer + 0) = 0;
        *(int *)(buffer + 4) = 0;
        *(int *)(buffer + 8) = 0;
        *(int *)(buffer + 12) = 0;
        *(short *)(buffer + 16) = 0;
        buffer[2] = 2;
        buffer[12] = (byte)width;
        buffer[13] = (byte)(width >> 8);
        buffer[14] = (byte)height;
        buffer[15] = (byte)(height >> 8);
        buffer[16] = 24;
        buffer[17] = 32;

        if (R_GetFrontBufferData(width, height, 3, buffer + 18)) {
            ri.FS_WriteFile(filename, buffer, fileSize);
        }
        ri.Z_FreeInternal(buffer);
    }

    if (!silent) {
        ri.Printf(0, "Wrote %s\n", filename);
    }
}

void R_CapturePendingScreenshotBeforePresent(void)
{
    char filename[256];
    int type;
    int silent;

    if (pendingScreenshotType < 0)
        return;

    if (pendingScreenshotDelayFrames > 0) {
        ri.Printf(0, "[screenshot-trace] pending delay before present frames=%d file='%s'\n",
                  pendingScreenshotDelayFrames, pendingScreenshotFilename);
        --pendingScreenshotDelayFrames;
        return;
    }

    type = pendingScreenshotType;
    silent = pendingScreenshotSilent;
    I_strncpyz(filename, pendingScreenshotFilename, sizeof(filename));
    pendingScreenshotType = -1;
    pendingScreenshotFilename[0] = '\0';
    pendingScreenshotSilent = 0;
    pendingScreenshotDelayFrames = 0;

    R_CaptureScreenshotNow(type, filename, silent);
}

int R_HasPendingScreenshot(void)
{
    return pendingScreenshotType >= 0;
}

void R_ScreenshotCommand(GfxScreenshotType type)
{
    char filename[256];
    char jpgFilename[256];
    const char *extension;
    int silent;
    byte *buffer;
    byte *pixels;
    const char *arg;

    if (type == 0) {
        extension = "jpg";
    } else if (type == 1) {
        extension = "tga";
    } else {
        return;
    }

    ri.Printf(0, "[screenshot-trace] command type=%d argc=%d arg1='%s'\n",
              type, ri.Cmd_Argc(), ri.Cmd_Argv(1));

    arg = ri.Cmd_Argv(1);
    if (strcmp(arg, "levelshot") == 0) {
        R_LevelShot();
        return;
    }

    arg = ri.Cmd_Argv(1);
    if (strcmp(arg, "savegame") == 0 && ri.Cmd_Argc() >= 3) {
        arg = ri.Cmd_Argv(2);
        if (arg[0] != '\0') {

            sprintf(jpgFilename, "%s.jpg", ri.Cmd_Argv(2));
            pixels = R_TakeResampledScreenshot(512, 512, 3, 0);
            if (!pixels) {
                return;
            }
            R_SaveJpg(jpgFilename, 90, 512, 512, pixels);
            ri.Z_FreeInternal(pixels);
            return;
        }
    }

    arg = ri.Cmd_Argv(1);
    silent = (strcmp(arg, "silent") == 0);

    if (ri.Cmd_Argc() == 2 && !silent) {
        Com_sprintf(filename, 256, "screenshots/%s.%s", ri.Cmd_Argv(1), extension);
    } else {

        int num = lastNumber;
        if (num <= 9999) {
            do {
                if (num > 9999) {
                    Com_sprintf(filename, 256, "screenshots/shot9999.%s", extension);
                } else {
                    Com_sprintf(filename, 256, "screenshots/shot%04i.%s", num, extension);
                }
                if (!ri.FS_FileExists(filename)) {
                    break;
                }
                num++;
                lastNumber = num;
            } while (num <= 9999);
        }

        if (lastNumber > 9998) {
            ri.Printf(0, "ScreenShot: Couldn't create a file\n");
            return;
        }
        lastNumber++;
    }

    pendingScreenshotType = type;
    pendingScreenshotSilent = silent;
    pendingScreenshotDelayFrames = 2;
    I_strncpyz(pendingScreenshotFilename, filename, sizeof(pendingScreenshotFilename));
    ri.Printf(0, "[screenshot-trace] queued '%s' type=%d\n", filename, type);
}
