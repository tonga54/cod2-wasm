#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"

#include <math.h>
#include <string.h>

extern GfxWorld s_world;
extern struct r_globals_load_t rgl;
extern refimport_t ri;
extern void *imp_r_optimizeLightmaps;
extern void *imp_vidConfig;

extern void R_Error(int level, const char *msg, ...);
extern void *Hunk_AllocInternal(int size);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);
extern dheader_t *Com_GetBsp(int *fileSize, unsigned int *checksum);
extern const char *Com_SkipPath(const char *path);
extern void Com_StripExtension(const char *in, char *out);
extern void I_strncpyz(char *dest, const char *src, int size);
extern void Com_Memset(void *dest, int val, int count);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void Com_Printf(const char *fmt, ...);
extern const char *va(const char *fmt, ...);
extern void *Image_Alloc(const char *name, int type1, int type2, int type3);
extern void Image_Generate2D(void *image, byte *data, int width, int height, int format);
extern const char *R_ParseSunLight(SunLightParseParams *params, const char *text);
extern void R_InterpretSunLightParseParamsIntoLights(SunLightParseParams *sunParse, GfxLight *sunLight);
extern void R_LoadSun(const char *name, sunflare_t *sun);
extern void R_RegisterOutdoorImage(GfxWorld *world);

extern void __attribute_regparm__(1) R_LoadSurfaces(GfxBspLoad *load);
extern void __attribute_regparm__(1) R_LoadCullGroups(const int *load);
extern void __attribute_regparm__(1) R_LoadPortalVerts(const int *load);
extern void __attribute_regparm__(1) R_LoadOccluders(const byte *load);
extern void __attribute_regparm__(1) R_LoadAabbTrees(const int *load);
extern void __attribute_regparm__(1) R_LoadCells(const int *load);
extern void __attribute_regparm__(1) R_LoadPortals(const int *load);
extern void __attribute_regparm__(1) R_LoadNodesAndLeafs(const byte *load);
extern void __attribute_regparm__(1) R_LoadEntities(GfxBspLoad *load);

typedef void (*loader_regparm_fn)(void *) __attribute_regparm__(1);
#define CALL_LOADER_EAX(func, arg) \
    ((loader_regparm_fn)(func))((void *)(arg))

#define LIGHTMAP_NONE 31
#define MAX_LIGHTMAPS 31
#define LIGHTMAP_PIXELS (1024 * 1024)
#define LIGHTMAP_BYTES (LIGHTMAP_PIXELS * 4)
#define MAX_MERGED_GROUPS 32

/* The sun-visibility plane has twice the baked RGB resolution. Filter its
 * complete 2x2 footprint once at map load instead of aliasing shadow edges
 * with a single top-left sample. The uploaded size and draw shaders stay the
 * same; this adds no work to gameplay frames. */
static float R_FilterSunVisibility(const byte *pixel, int rowBytes)
{
    return (pixel[0] + pixel[1] + pixel[rowBytes] + pixel[rowBytes + 1]) * 0.25f;
}

static void R_PrintLoadProgress(const char *what)
{
    ri.Printf(0, "Loading %s...\n", what);
    ri.Cbuf_ExecuteText(0, "updatescreen\n");
}

static int R_ValidateLumpRaw(const byte *header, const byte *fileBase, int fileSize,
                             int lumpByteOfs, int elemSize, int *countOut, const byte **dataOut)
{
    int lumpSize = *(int *)(header + lumpByteOfs);
    int lumpFileOfs = *(int *)(header + lumpByteOfs + 4);
    int count;

    if (lumpFileOfs + lumpSize > fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (lumpFileOfs <= 3) {
        Com_Printf("DBG funny lump offset: byteOfs=0x%x size=%d fileOfs=%d\n", lumpByteOfs, lumpSize, lumpFileOfs);

        if (lumpSize == 0) {
            if (countOut)
                *countOut = 0;
            if (dataOut)
                *dataOut = NULL;
            return 0;
        }
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);
    }

    if (lumpSize < 0)
        count = (lumpSize + elemSize - 1) / elemSize;
    else
        count = lumpSize / elemSize;

    if (count * elemSize != lumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);

    if (countOut)
        *countOut = count;
    if (dataOut)
        *dataOut = fileBase + lumpFileOfs;
    return lumpSize;
}

#define LUMP_OFS_MATERIALS 0x08
#define LUMP_OFS_LIGHTMAPS 0x10
#define LUMP_OFS_GRIDPOINTS 0x18
#define LUMP_OFS_GRIDCOLORS 0x20
#define LUMP_OFS_TRIANGLES 0x40
#define LUMP_OFS_CULLGROUPIDX 0x60
#define LUMP_OFS_OCCLUDERS 0xA8
#define LUMP_OFS_OCCLUDERIDX 0xB0
#define LUMP_OFS_SUBMODELS 0x120
#define LUMP_OFS_ENTITIES 0x130

GfxWorld *R_LoadWorldInternal(const char *name)
{

    GfxBspLoad load;
    char baseName[64];
    const byte *header;
    const byte *fileBase;

    r_lightmapGroup_t groupInfo[MAX_LIGHTMAPS];
    int reorder[MAX_LIGHTMAPS];
    int coupling[MAX_LIGHTMAPS][MAX_LIGHTMAPS];
    int lmapVertCount[MAX_LIGHTMAPS];
    byte used[MAX_LIGHTMAPS];
    int origLmapCount;
    int wideCount, highCount;
    int triSoupCount;
    const DiskTriangleSoup *triSurfs;
    const byte *lmapData;
    int lmapDataSize;

    byte *imageBuffer[4];
    int subImgBytesPerPixel[4];
    int subImgWidth[4];
    int subImgHeight[4];

    int modelCount;

    int i, j, k;
    int nameLen;

    memset(&s_world, 0, sizeof(GfxWorld));
    memset(&rgl, 0, 6 * 4);

    header = (const byte *)Com_GetBsp(&load.fileSize, &s_world.checksum);
    fileBase = (const byte *)header;

    load.header = (const dheader_t *)header;
    load.fileBase = fileBase;

    nameLen = strlen(name) + 1;
    s_world.name = (const char *)Hunk_AllocInternal(nameLen);
    strcpy((char *)s_world.name, name);

    I_strncpyz(baseName, Com_SkipPath(s_world.name), 64);
    Com_StripExtension(baseName, baseName);
    nameLen = strlen(baseName) + 1;
    s_world.baseName = (const char *)Hunk_AllocInternal(nameLen);
    strcpy((char *)s_world.baseName, baseName);

    R_PrintLoadProgress("materials");
    {
        const byte *mtlData;
        int mtlCount;
        R_ValidateLumpRaw(header, fileBase, load.fileSize,
                          LUMP_OFS_MATERIALS, 72, &mtlCount, &mtlData);
        load.materialCount = mtlCount;
        load.materials = (const dmaterial_t *)mtlData;
    }

    R_PrintLoadProgress("sun settings");
    {

        int entityOfs = *(int *)(header + LUMP_OFS_ENTITIES + 4);
        const char *entityString = (const char *)(fileBase + entityOfs);
        R_ParseSunLight(&s_world.sunParse, entityString);
        R_InterpretSunLightParseParamsIntoLights(&s_world.sunParse, &s_world.sunLight);
    }

    R_PrintLoadProgress("lightmaps");

    float sunR = s_world.sunLight.color[0];
    float sunG = s_world.sunLight.color[1];
    float sunB = s_world.sunLight.color[2];

    load.lmapMergeInfo[LIGHTMAP_NONE].index = LIGHTMAP_NONE;
    load.lmapMergeInfo[LIGHTMAP_NONE].shift[0] = 0.0f;
    load.lmapMergeInfo[LIGHTMAP_NONE].shift[1] = 0.0f;
    load.lmapMergeInfo[LIGHTMAP_NONE].scale[0] = 1.0f;
    load.lmapMergeInfo[LIGHTMAP_NONE].scale[1] = 1.0f;

    R_ValidateLumpRaw(header, fileBase, load.fileSize,
                      LUMP_OFS_TRIANGLES, 16, &triSoupCount, (const byte **)&triSurfs);

    lmapDataSize = R_ValidateLumpRaw(header, fileBase, load.fileSize,
                                     LUMP_OFS_LIGHTMAPS, LIGHTMAP_BYTES,
                                     NULL, &lmapData);
    int lmapPageCount = lmapDataSize / LIGHTMAP_BYTES;

    if (triSoupCount > 0) {
        origLmapCount = 0;
        for (i = 0; i < triSoupCount; i++) {
            int lmIdx = (int)(unsigned short)triSurfs[i].lightmapIndex;
            if ((short)lmIdx != LIGHTMAP_NONE) {
                int candidate = (int)(signed short)lmIdx + 1;
                if (candidate > origLmapCount)
                    origLmapCount = candidate;
            }
        }
    } else {
        origLmapCount = 0;
    }

    if (lmapPageCount != 0 && lmapPageCount != origLmapCount)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);

    memset(lmapVertCount, 0, sizeof(int) * MAX_LIGHTMAPS);
    memset(coupling, 0, sizeof(int) * MAX_LIGHTMAPS * MAX_LIGHTMAPS);

    for (int materialIndex = 0; materialIndex < load.materialCount; materialIndex++) {

        for (i = 0; i < triSoupCount; i++) {
            if ((signed short)triSurfs[i].materialIndex != materialIndex)
                continue;
            int lmIdx = (int)(unsigned short)triSurfs[i].lightmapIndex;
            if ((short)lmIdx == LIGHTMAP_NONE)
                continue;
            lmapVertCount[lmIdx] += (signed short)triSurfs[i].vertexCount;
        }

        for (int lmapIndex = 0; lmapIndex < origLmapCount; ) {
            if (lmapVertCount[lmapIndex] == 0) {
                lmapIndex++;
                continue;
            }

            int nextLmap = lmapIndex + 1;
            if (nextLmap < origLmapCount) {
                for (j = nextLmap; j < origLmapCount; j++) {
                    if (lmapVertCount[j] == 0)
                        continue;
                    int strength = lmapVertCount[j] + lmapVertCount[lmapIndex] + coupling[lmapIndex][j];
                    if (strength < 0)
                        strength = 0x7FFFFFFF;
                    coupling[lmapIndex][j] = strength;
                    coupling[j][lmapIndex] = strength;
                }
            }

            lmapVertCount[lmapIndex] = 0;

            lmapIndex = nextLmap;
            while (lmapIndex < origLmapCount && lmapVertCount[lmapIndex] == 0)
                lmapIndex++;
        }
    }

    memset(used, 0, sizeof(used));

    wideCount = 1;
    highCount = 1;

    int newLmapCount;
    int usedCount;
    r_lightmapGroup_t *groupPtr;

    if (origLmapCount <= 0) {
        newLmapCount = 0;
    } else {
        usedCount = 0;
        newLmapCount = 0;
        groupPtr = groupInfo;
        int remaining = origLmapCount;

        while (usedCount < origLmapCount) {
            remaining = origLmapCount - usedCount;

            int targetSize = wideCount * highCount;
            while (remaining < targetSize) {
                if (wideCount < highCount) {
                    highCount >>= 1;
                } else {
                    wideCount >>= 1;
                }
                targetSize = wideCount * highCount;
            }

            if (targetSize <= 1) {

                int found = -1;
                if (!used[0]) {
                    found = 0;
                } else {
                    for (i = 1; i < origLmapCount; i++) {
                        if (!used[i]) {
                            found = i;
                            break;
                        }
                    }
                }
                if (found < 0) {

                    break;
                }
                reorder[usedCount] = found;
                usedCount++;
                used[found] = 1;
            } else {

                int firstLmap = -1;
                int bestOther = LIGHTMAP_NONE;

                for (i = 0; i < origLmapCount; i++) {
                    if (!used[i]) {
                        firstLmap = i;
                        break;
                    }
                }

                bestOther = LIGHTMAP_NONE;
                for (j = firstLmap + 1; j < origLmapCount; j++) {
                    if (used[j])
                        continue;
                    if (bestOther == LIGHTMAP_NONE ||
                        coupling[firstLmap][j] > coupling[firstLmap][bestOther]) {
                        bestOther = j;
                    }
                }

                reorder[usedCount] = firstLmap;
                reorder[usedCount + 1] = bestOther;
                usedCount += 2;
                used[firstLmap] = 1;
                used[bestOther] = 1;

                if (targetSize > 2) {
                    int slotIdx = usedCount;
                    for (int filled = 2; filled < targetSize; filled++) {

                        for (k = 0; k < origLmapCount; k++) {
                            coupling[bestOther][k] += coupling[firstLmap][k];
                            coupling[k][bestOther] = coupling[bestOther][k];
                        }

                        int nextBest = LIGHTMAP_NONE;
                        for (k = 0; k < origLmapCount; k++) {
                            if (used[k])
                                continue;
                            if (nextBest == LIGHTMAP_NONE ||
                                coupling[bestOther][k] > coupling[bestOther][nextBest]) {
                                nextBest = k;
                            }
                        }

                        reorder[slotIdx] = nextBest;
                        slotIdx++;
                        usedCount++;
                        used[nextBest] = 1;

                        firstLmap = nextBest;
                    }
                }
            }

            groupPtr->wideCount = wideCount;
            groupPtr->highCount = highCount;
            newLmapCount++;
            groupPtr++;

        }
    }

    Com_Printf("%i merged lightmaps from %i original lightmaps\n", newLmapCount, origLmapCount);

    if (origLmapCount == 0) {
        s_world.lightmapCount = 0;
    } else {

        for (i = 0; i < 3; i++) {
            subImgBytesPerPixel[i] = 4;
            subImgWidth[i] = 512;
            subImgHeight[i] = 512;
        }
        subImgBytesPerPixel[3] = 1;
        subImgWidth[3] = 1024;
        subImgHeight[3] = 1024;

        int firstGroupWide = groupInfo[0].wideCount;
        int firstGroupHigh = groupInfo[0].highCount;
        int totalImageSize = firstGroupWide * firstGroupHigh * LIGHTMAP_BYTES;

        imageBuffer[0] = (byte *)Hunk_AllocateTempMemoryInternal(totalImageSize);

        for (i = 1; i < 4; i++) {
            int planeSize = subImgBytesPerPixel[i] * subImgWidth[i] * subImgHeight[i] * firstGroupWide * firstGroupHigh;
            imageBuffer[i] = imageBuffer[i - 1] + planeSize;
        }

        {
            int lmDataOfs = *(int *)(header + LUMP_OFS_LIGHTMAPS + 4);
            lmapData = fileBase + lmDataOfs;
            lmapDataSize = *(int *)(header + LUMP_OFS_LIGHTMAPS);
        }

        if (lmapDataSize == 0)
            memset(imageBuffer[0], 0xFF, totalImageSize);

        s_world.lightmaps = (GfxLightmapArray *)Hunk_AllocInternal(MAX_MERGED_GROUPS * sizeof(GfxLightmapArray));

        int groupIdx = 0;
        if (origLmapCount > 0) {

            byte *hiResBase = imageBuffer[3];
            byte *loResBase1 = imageBuffer[0];
            byte *loResBase2 = imageBuffer[1];
            byte *loResBase3 = imageBuffer[2];

            int oldLmapBaseIndex = 0;

            while (oldLmapBaseIndex < origLmapCount) {
                short mergedIndex = (short)groupIdx;
                int grpWide = groupInfo[groupIdx].wideCount;
                int grpHigh = groupInfo[groupIdx].highCount;
                int groupTileCount = grpWide * grpHigh;

                for (int tileIndex = 0; tileIndex < groupTileCount; tileIndex++) {
                    int oldLmapIndex = reorder[oldLmapBaseIndex + tileIndex];
                    int tileCol = tileIndex % grpWide;
                    int tileRow = tileIndex / grpWide;

                    if (lmapDataSize != 0) {

                        const byte *srcPtr = lmapData + (unsigned int)oldLmapIndex * LIGHTMAP_BYTES;

                        for (int subImg = 0; subImg < 4; subImg++) {
                            int curWide = grpWide;
                            int bpp = subImgBytesPerPixel[subImg];
                            int srcH = subImgHeight[subImg];
                            int srcW = subImgWidth[subImg];

                            int dstRowStride = curWide * srcW * bpp;
                            byte *dstBase = imageBuffer[subImg] + ((tileRow * srcH * curWide * srcW) + (tileCol * srcW)) * bpp;

                            if (srcH > 0) {
                                int srcRowBytes = srcW * bpp;
                                for (int row = 0; row < srcH; row++) {
                                    Com_Memcpy(dstBase, srcPtr, srcRowBytes);
                                    srcPtr += srcRowBytes;
                                    dstBase += dstRowStride;
                                }
                            }

                            grpWide = groupInfo[groupIdx].wideCount;
                        }
                    }

                    load.lmapMergeInfo[oldLmapIndex].index = mergedIndex;
                    load.lmapMergeInfo[oldLmapIndex].scale[0] = 1.0f / (float)grpWide;
                    load.lmapMergeInfo[oldLmapIndex].scale[1] = 1.0f / (float)grpHigh;
                    load.lmapMergeInfo[oldLmapIndex].shift[0] = (float)tileCol * load.lmapMergeInfo[oldLmapIndex].scale[0];
                    load.lmapMergeInfo[oldLmapIndex].shift[1] = (float)tileRow * load.lmapMergeInfo[oldLmapIndex].scale[1];
                }

                int rendererInUse = r_rendererInUse->current.integer;

#ifdef GFX_REAL_D3D9

                if (0) {

#else
                /* DX7's sampler.lightmap.traditional consumes a single baked
                 * RGB image. Directional coefficients belong to the DX9 path. */
                if (rendererInUse == 2) {

#endif
                    int grpOffset = groupIdx * (int)sizeof(GfxLightmapArray);

                    ((GfxImage **)((byte *)s_world.lightmaps + grpOffset))[0] =
                        (GfxImage *)Image_Alloc(va("*lightmap%i", groupIdx), 2, 1, 4);

                    int pixWidth = grpWide * 512;
                    int pixHeight = grpHigh * 512;

                    if (pixHeight > 0) {

                        int loResPixelOfs = 0;

                        for (int y = 0; y < pixHeight; y++) {
                            if (pixWidth <= 0)
                                continue;

                            byte *hiResPtr = hiResBase + y * pixWidth * 4;
                            byte *loR = loResBase1 + loResPixelOfs;
                            byte *loG = loResBase2 + loResPixelOfs;
                            byte *loB = loResBase3 + loResPixelOfs;

                            for (int x = 1; x <= pixWidth; x++) {

                                float sunVis = R_FilterSunVisibility(hiResPtr, pixWidth * 2);

                                float valR = (float)(unsigned char)(*loR) + sunR * sunVis;
                                valR = floorf(valR * 0.5f + 0.5f);
                                int iR = (int)valR;
                                if (iR < 0)
                                    iR = 0;
                                if (iR > 255)
                                    iR = 255;

                                float valG = (float)(unsigned char)loG[x * 4 - 4] + sunG * sunVis;
                                valG = floorf(valG * 0.5f + 0.5f);
                                int iG = (int)valG;
                                if (iG < 0)
                                    iG = 0;
                                if (iG > 255)
                                    iG = 255;

                                float valB = (float)(unsigned char)loB[x * 4 - 4] + sunB * sunVis;
                                valB = floorf(valB * 0.5f + 0.5f);
                                int iB = (int)valB;
                                if (iB < 0)
                                    iB = 0;
                                if (iB > 255)
                                    iB = 255;

                                loR[0] = (byte)iB;
                                loR[1] = (byte)iG;
                                loR[2] = (byte)iR;
                                loR[3] = 0xFF;

                                hiResPtr += 2;
                                loR += 4;
                            }

                            loResPixelOfs += pixWidth * 4;
                        }
                    }

                    Image_Generate2D(
                        ((GfxImage **)((byte *)s_world.lightmaps + grpOffset))[0],
                        loResBase1,
                        pixWidth, pixHeight, 0x15);

                } else {

                    int grpOffset = groupIdx * (int)sizeof(GfxLightmapArray);

                    for (int si = 0; si < 4; si++) {
                        ((GfxImage **)((byte *)s_world.lightmaps + grpOffset))[si] =
                            (GfxImage *)Image_Alloc(
                                va("*lightmap%i_%i", groupIdx, si), 2, 1, 4);

                        int format = (subImgBytesPerPixel[si] == 4) ? 0x15 : 0x32;

                        int imgW = subImgWidth[si] * grpWide;
                        int imgH = subImgHeight[si] * grpHigh;

                        Image_Generate2D(
                            ((GfxImage **)((byte *)s_world.lightmaps + grpOffset))[si],
                            imageBuffer[si],
                            imgW, imgH, format);
                    }
                }

                oldLmapBaseIndex += groupTileCount;
                groupIdx++;
            }
        }

        s_world.lightmapCount = (origLmapCount > 0) ? (short)groupIdx : 0;
        Hunk_FreeTempMemory(imageBuffer[0]);
    }

    R_PrintLoadProgress("lightgrid coefficients");
    {
        const byte *gridColorData;
        int gridColorCount;
        int lumpSize = R_ValidateLumpRaw(header, fileBase, load.fileSize,
                                         LUMP_OFS_GRIDCOLORS, 24,
                                         &gridColorCount, &gridColorData);

        s_world.lightGrid.colorCount = gridColorCount;
        s_world.lightGrid.colors = (GfxLightGridColors *)Hunk_AllocInternal(lumpSize);
        Com_Memcpy(s_world.lightGrid.colors, gridColorData, lumpSize);
    }

    R_PrintLoadProgress("lightgrid hash");
    {
        const byte *gridPointData;
        int gridPointCount;
        R_ValidateLumpRaw(header, fileBase, load.fileSize,
                          LUMP_OFS_GRIDPOINTS, 8,
                          &gridPointCount, &gridPointData);

        s_world.lightGrid.pointCount = gridPointCount;
        GfxLightGridPoint *pts = (GfxLightGridPoint *)Hunk_AllocInternal(gridPointCount * 8);
        s_world.lightGrid.points = pts;

        const GfxLightGridPoint *srcPts = (const GfxLightGridPoint *)gridPointData;
        for (i = 0; i < s_world.lightGrid.pointCount; i++) {
            pts[i].xyzHighBits = srcPts[i].xyzHighBits;
            pts[i].xyzLowBitsAndSunVisible = srcPts[i].xyzLowBitsAndSunVisible;
            pts[i].needsTrace = srcPts[i].needsTrace;
            pts[i].colorsIndex = srcPts[i].colorsIndex;
        }
    }

    R_PrintLoadProgress("surfaces");
    CALL_LOADER_EAX(R_LoadSurfaces, &load);

    R_PrintLoadProgress("cull groups");
    CALL_LOADER_EAX(R_LoadCullGroups, &load);

    R_PrintLoadProgress("cull group indices");
    {
        const byte *cgIdxData;
        int cgIdxCount;
        R_ValidateLumpRaw(header, fileBase, load.fileSize,
                          LUMP_OFS_CULLGROUPIDX, 4,
                          &cgIdxCount, &cgIdxData);

        const int *srcIdx = (const int *)cgIdxData;
        int *dstIdx = (int *)Hunk_AllocInternal(cgIdxCount * 4);
        rgl.cullGroupIndices = dstIdx;

        for (i = 0; i < cgIdxCount; i++)
            dstIdx[i] = srcIdx[i];
    }

    R_PrintLoadProgress("portal vertices");
    CALL_LOADER_EAX(R_LoadPortalVerts, &load);

    R_PrintLoadProgress("occluders");
    {

        int occLumpSize = *(int *)(header + LUMP_OFS_OCCLUDERS);
        if (occLumpSize > 0)
            CALL_LOADER_EAX(R_LoadOccluders, &load);
    }

    R_PrintLoadProgress("occluder indices");
    {
        const byte *occIdxData;
        int occIdxCount;
        R_ValidateLumpRaw(header, fileBase, load.fileSize,
                          LUMP_OFS_OCCLUDERIDX, 2,
                          &occIdxCount, &occIdxData);

        const short *srcOcc = (const short *)occIdxData;
        GfxOccluder **dstOcc = (GfxOccluder **)Hunk_AllocInternal(occIdxCount * 4);
        rgl.occluderIndices = dstOcc;

        for (i = 0; i < occIdxCount; i++) {
            int idx = (int)(signed short)srcOcc[i];

            dstOcc[i] = (GfxOccluder *)((byte *)rgl.occluders + idx * 36);
        }
    }

    R_PrintLoadProgress("AABB trees");
    CALL_LOADER_EAX(R_LoadAabbTrees, &load);

    R_PrintLoadProgress("cells");
    CALL_LOADER_EAX(R_LoadCells, &load);

    R_PrintLoadProgress("portals");
    CALL_LOADER_EAX(R_LoadPortals, &load);

    R_PrintLoadProgress("nodes and leafs");
    CALL_LOADER_EAX(R_LoadNodesAndLeafs, &load);

    R_PrintLoadProgress("submodels");
    {
        const byte *modelData;
        R_ValidateLumpRaw(header, fileBase, load.fileSize,
                          LUMP_OFS_SUBMODELS, 48,
                          &modelCount, &modelData);

        const dmodel_t *inModels = (const dmodel_t *)modelData;
        GfxBrushModel *outModels = (GfxBrushModel *)Hunk_AllocInternal(modelCount * sizeof(GfxBrushModel));
        s_world.models = outModels;
        s_world.modelCount = modelCount;

        for (i = 0; i < modelCount; i++) {

            for (j = 0; j < 3; j++) {
                outModels[i].bounds[0][j] = inModels[i].mins[j];
                outModels[i].bounds[1][j] = inModels[i].maxs[j];
            }

            outModels[i].surfaceCount = inModels[i].numTriangles;

            if (inModels[i].numTriangles != 0)
                outModels[i].startSurfIndex = inModels[i].firstTriangle;
            else
                outModels[i].startSurfIndex = -1;
        }
    }

    R_PrintLoadProgress("entities");
    R_LoadEntities(&load);

    memset(&rgl, 0, 6 * 4);

    R_LoadSun(name, &s_world.sun);
    R_RegisterOutdoorImage(&s_world);

    return &s_world;
}
