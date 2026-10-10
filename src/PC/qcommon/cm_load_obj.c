#include "common_types.h"
#include "imports.h"

extern float sqrtf(float x);

typedef struct dctri_ondisk_s {
    float v[12];
    int verts[3];
    int edges[3];
} dctri_ondisk_t;

typedef struct dcborder_ondisk_s {
    float v[7];
} dcborder_ondisk_t;

typedef struct dcedge_ondisk_s {
    int prefix;
    float v[12];
    float invLen;
} dcedge_ondisk_t;

typedef struct dleafbrush_ondisk_s {
    int reserved[4];
    int indexFirstLeafBrush;
    int numLeafBrushes;
    int reserved2[3];
} dleafbrush_ondisk_t;

typedef struct dnode_ondisk_s {
    int planeNum;
    int children[2];
    int reserved[6];
} dnode_ondisk_t;

typedef struct dsubmodel_ondisk_s {
    float mins[3];
    float maxs[3];
    int reserved[2];
    int firstBrush;
    int numBrushes;
    int reserved2[2];
} dsubmodel_ondisk_t;

typedef struct dleaf_ondisk_s {
    int cluster;
    int area;
    int firstCollAabbIndex;
    int collAabbCount;
    int reserved[5];
} dleaf_ondisk_t;

typedef struct cml_s {
    int numPlanes;
    cplane_t *planes;
    void *base;
    char _pad[124 - 12];
} cml_t;

static cml_t cml;

extern clipMap_t cm;
#define cm_ptr ((clipMap_t *)&cm)

void CM_Cleanup(void);
cplane_t *CM_GetPlaneNum(int planeNum);
void CMod_LoadPlanes(const byte *base, const lump_t *l);
static cLeafBrushNode_t *CMod_PartionLeafBrushes_r(unsigned short *leafBrushes, int numLeafBrushes, const vec_t *mins, const vec_t *maxs);
static void CMod_PartionLeafBrushes(unsigned short *leafBrushes, int numLeafBrushes, cLeaf_t *leaf);
void CM_LoadMapFromBsp(const char *name, int usePvs);

extern void *CM_Hunk_Alloc(int size, const char *name, int type);
extern void CM_Hunk_CheckTempMemoryClear(void);
extern void CM_Hunk_CheckTempMemoryHighClear(void);
extern void *CM_Hunk_AllocateTempMemoryHigh(int size, const char *name);
extern void CM_Hunk_ClearTempMemory(void);
extern void CM_Hunk_ClearTempMemoryHigh(void);
extern char *TempMalloc(int len);
extern void TempMemoryReset(void);
extern const dheader_t *Com_GetBsp(int *fileSize, unsigned int *checksum);
extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern void Com_Memset(void *dest, int val, int count);
extern void Com_Memcpy(void *dest, const void *src, int count);

void CM_Cleanup(void)
{
    cml.base = (void *)0;
}

cplane_t *CM_GetPlaneNum(int planeNum)
{
    return cml.planes + planeNum;
}

void CMod_LoadPlanes(const byte *base, const lump_t *l)
{
    int i, j;
    int count;
    const byte *in;
    cplane_t *out;
    byte bits;

    in = base + l->fileofs;

    if (l->filelen & 0xf) {
        Com_Error(ERR_DROP, "CMod_LoadPlanes: funny lump size");
    }

    count = l->filelen >> 4;

    if (count <= 0) {
        Com_Error(ERR_DROP, "CMod_LoadPlanes: map has no planes");
    }

    out = (cplane_t *)CM_Hunk_Alloc(count * sizeof(cplane_t), "CMod_LoadPlanes", 0x17);
    cml.planes = out;
    cml.numPlanes = count;

    for (i = 0; i < count; i++) {
        bits = 0;
        for (j = 0; j < 3; j++) {
            out->normal[j] = ((const cplane_t *)in)->normal[j];
            if (0.0f > out->normal[j]) {
                bits |= (1 << j);
            }
        }
        out->dist = ((const cplane_t *)in)->dist;

        if (out->normal[0] == 1.0f) {
            out->type = 0;
        } else if (out->normal[1] == 1.0f) {
            out->type = 1;
        } else {
            byte t = (out->normal[2] != 1.0f) | ((*(unsigned int *)&out->normal[2] ^ 0x3f800000) ? 1 : 0);

            if (out->normal[2] == 1.0f) {
                out->type = 2;
            } else {
                out->type = 3;
            }
        }

        out->signbits = bits;
        in += 16;
        out++;
    }
}

static cLeafBrushNode_t *CMod_PartionLeafBrushes_r(unsigned short *leafBrushes, int numLeafBrushes, const vec_t *mins, const vec_t *maxs)
{
    cLeafBrushNode_t *node;
    int k;

    (void)mins;
    (void)maxs;
    /* This loader deliberately emits a flat leaf (the old implementation
     * forced bestAxis = -1 after computing every candidate split). Keep that
     * exact layout and brush order without the discarded quadratic scoring. */
    node = (cLeafBrushNode_t *)TempMalloc(sizeof(cLeafBrushNode_t));
    node->axis = 0;
    node->contents = 0;
    node->data.children.range = 0;
    node->data.children.childOffset[0] = 0;
    node->data.children.childOffset[1] = 0;
    node->leafBrushCount = (short)numLeafBrushes;
    if ((int)(short)numLeafBrushes != numLeafBrushes)
        Com_Error(ERR_DROP, "CMod_PartionLeafBrushes_r: numLeafBrushes overflows a short");
    for (k = 0; k < numLeafBrushes; k++)
        node->contents |= cm_ptr->brushes[leafBrushes[k]].contents;
    node->data.leaf.brushes = leafBrushes;
    return node;
}

static void CMod_PartionLeafBrushes(unsigned short *leafBrushes, int numLeafBrushes, cLeaf_t *leaf)
{
    int k, j;
    vec3_t mins, maxs;
    cbrush_t *brushesBase;
    cLeafBrushNode_t *resultNode;

    if (numLeafBrushes == 0) {
        return;
    }

    mins[0] = 3.4028234663852886e+38f;
    mins[1] = 3.4028234663852886e+38f;
    mins[2] = 3.4028234663852886e+38f;
    maxs[0] = -3.4028234663852886e+38f;
    maxs[1] = -3.4028234663852886e+38f;
    maxs[2] = -3.4028234663852886e+38f;

    if (numLeafBrushes > 0) {
        brushesBase = cm_ptr->brushes;
        for (k = 0; k < numLeafBrushes; k++) {
            cbrush_t *b = brushesBase + leafBrushes[k];

            for (j = 0; j < 3; j++) {
                float bmin = ((float *)&b->mins)[j];
                float bmax = ((float *)&b->maxs)[j];
                if (bmin < mins[j]) {
                    mins[j] = bmin;
                }
                if (bmax > maxs[j]) {
                    maxs[j] = bmax;
                }
            }
        }
    }

    leaf->mins[0] = mins[0];
    leaf->mins[1] = mins[1];
    leaf->mins[2] = mins[2];
    leaf->maxs[0] = maxs[0];
    leaf->maxs[1] = maxs[1];
    leaf->maxs[2] = maxs[2];

    for (j = 0; j < 3; j++) {
        leaf->mins[j] -= 0.125f;
        leaf->maxs[j] += 0.125f;
    }

    CM_Hunk_CheckTempMemoryHighClear();

    resultNode = CMod_PartionLeafBrushes_r(leafBrushes, numLeafBrushes, mins, maxs);

    {
        int byteOffset = (int)((byte *)resultNode - (byte *)cm_ptr->leafbrushNodes);
        int dwordOffset = byteOffset >> 2;
        int t = dwordOffset * 3;
        t = t + (t << 4);
        t = t + (t << 8);
        t = t + (t << 16);
        leaf->leafBrushNode = dwordOffset + t * 4;
    }

    CM_Hunk_ClearTempMemoryHigh();
}

static int CM_GetLeafTerrainContents(cLeaf_t *leaf)
{
    int contents = 0;
    int count = leaf->collAabbCount;
    int k;
    clipMap_t *cmLocal = cm_ptr;

    if (count <= 0)
        return 0;

    {
        dmaterial_t *materials = cmLocal->materials;
        CollisionAabbTree *trees = cmLocal->aabbTrees + leaf->firstCollAabbIndex;

        for (k = 0; k < count; k++) {
            contents |= materials[trees[k].materialIndex].contentFlags;
        }
    }
    return contents;
}

void CM_LoadMapFromBsp(const char *name, int usePvs)
{
    clipMap_t *cmLocal;
    const dheader_t *header;
    const byte *bspBase;
    int count;
    int i, j;
    byte usePvsFlag;

    cmLocal = cm_ptr;
    usePvsFlag = (byte)usePvs;

    Com_Memset(cmLocal, 0, sizeof(clipMap_t));
    Com_Memset(&cml, 0, 12);

    {
        int nameLen = strlen(name) + 1;
        cmLocal->name = (const char *)CM_Hunk_Alloc(nameLen, "CM_LoadMapFromBsp", 0x17);
        strcpy((char *)cmLocal->name, name);
    }

    header = Com_GetBsp(NULL, &cmLocal->checksum);
    bspBase = (const byte *)header;
    cml.base = (void *)header;

    {
        const byte *in;
        int matLumpLen, matLumpOfs;

        matLumpOfs = header->lumps[0].fileofs;
        matLumpLen = header->lumps[0].filelen;
        in = bspBase + matLumpOfs;

        if (matLumpLen % 72 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadMaterials: funny lump size");
        }
        count = matLumpLen / 72;
        if (count <= 0) {
            Com_Error(ERR_DROP, "CMod_LoadMaterials: map has no materials");
        }

        cmLocal->materials = (dmaterial_t *)CM_Hunk_Alloc(count * 72 + 72, "CMod_LoadMaterials", 0x17);
        cmLocal->materials = (dmaterial_t *)((byte *)cmLocal->materials + 72);
        cmLocal->numMaterials = count;

        Com_Memcpy(cmLocal->materials, (const void *)in, count * 72);
    }

    CMod_LoadPlanes(bspBase, &header->lumps[4]);

    usePvsFlag = (byte)usePvs;

    {

        const byte *inBrush;
        const byte *inSides;
        int brushCount;
        int sideCount;
        int sideLumpLen;
        cbrush_t *outBrush;
        cbrushside_t *outSides;

        inBrush = bspBase + header->lumps[6].fileofs;
        if (header->lumps[6].filelen & 3) {
            Com_Error(ERR_DROP, "CMod_LoadBrushes: funny lump size");
        }
        brushCount = header->lumps[6].filelen >> 2;

        inSides = bspBase + header->lumps[5].fileofs;
        sideLumpLen = header->lumps[5].filelen;
        if (sideLumpLen & 7) {
            Com_Error(ERR_DROP, "CMod_LoadBrushes: funny lump size");
        }
        sideCount = sideLumpLen >> 3;
        if (sideCount - brushCount * 6 < 0) {
            Com_Error(ERR_DROP, "CMod_LoadBrushSides: too few sides");
        }

        if (sideCount == 0) {
            outSides = (cbrushside_t *)0;
        } else {
            outSides = (cbrushside_t *)CM_Hunk_Alloc(sideCount * 8, "CMod_LoadBrushSides", 0x18);
        }
        cmLocal->brushsides = outSides;
        cmLocal->numBrushSides = sideCount;

        outBrush = (cbrush_t *)CM_Hunk_Alloc((brushCount * 3 + 3) * 16, "CMod_LoadBrushes", 0x18);
        cmLocal->brushes = outBrush;
        cmLocal->numBrushes = (unsigned short)brushCount;
        if ((int)(unsigned short)brushCount != brushCount) {
            Com_Error(ERR_DROP, "CMod_LoadBrushes: numBrushes overflows a short");
        }

        for (i = 0; i < brushCount; i++) {
            int numSides;
            int materialNum;
            int axialIdx, sideIdx;

            numSides = (int)*(const short *)inBrush - 6;
            outBrush->numsides = numSides;
            if (numSides < 0) {
                Com_Error(ERR_DROP, "CMod_LoadBrushes: bad numsides");
            }

            if (outBrush->numsides == 0) {
                outBrush->sides = (cbrushside_t *)0;
            } else {
                outBrush->sides = outSides;
            }

            for (axialIdx = 0; axialIdx < 3; axialIdx++) {

                {
                    int val = *(const int *)inSides;

                    *(int *)&outBrush->mins[axialIdx] = val;
                }
                for (sideIdx = 0; sideIdx < 2; sideIdx++) {
                    if (sideIdx > 0) {
                        int val = *(const int *)inSides;
                        *(int *)&outBrush->maxs[axialIdx] = val;
                    }

                    materialNum = *(const int *)(inSides + 4);
                    if (materialNum < 0 || materialNum >= cmLocal->numMaterials) {
                        Com_Error(ERR_DROP, "CMod_LoadBrushes: bad materialNum: %i", materialNum);
                    }

                    outBrush->axialMaterialNum[sideIdx][axialIdx] = (short)materialNum;
                    if ((int)(short)materialNum != materialNum) {
                        Com_Error(ERR_DROP, "CMod_LoadBrushSides: materialNum overflows a short");
                    }

                    inSides += 8;
                }
            }

            {
                int *outSideNumSides = &outBrush->numsides;
                for (j = 0; j < *outSideNumSides; j++) {
                    int planeNum = *(const int *)inSides;

                    if ((unsigned)planeNum >= (unsigned)cml.numPlanes) {
                        Com_Error(ERR_DROP, "CMod_LoadBrushes: bad planeNum: %i", planeNum);
                    }
                    outSides->plane = cml.planes + planeNum;
                    outSides->materialNum = *(const int *)(inSides + 4);
                    if (outSides->materialNum < 0 || outSides->materialNum >= cmLocal->numMaterials) {
                        Com_Error(ERR_DROP, "CMod_LoadBrushes: bad materialNum: %i", outSides->materialNum);
                    }
                    inSides += 8;
                    outSides++;
                }
            }

            materialNum = (int)((const short *)inBrush)[1];
            if (materialNum < 0 || materialNum >= cmLocal->numMaterials) {
                Com_Error(ERR_DROP, "CMod_LoadBrushes: bad materialNum: %i", materialNum);
            }
            outBrush->contents = cmLocal->materials[materialNum].contentFlags & 0xDFFFFFFB;

            outBrush++;
            inBrush += 4;
        }
    }

    {
        const byte *in;
        unsigned short *out;

        in = bspBase + header->lumps[27].fileofs;
        if (header->lumps[27].filelen & 3) {
            Com_Error(ERR_DROP, "CMod_LoadLeafBrushes: funny lump size");
        }
        count = header->lumps[27].filelen >> 2;

        out = (unsigned short *)CM_Hunk_Alloc(count * 2 + 2, "CMod_LoadLeafBrushes", 0x18);
        cmLocal->leafbrushes = out;
        cmLocal->numLeafBrushes = count;

        for (i = 0; i < count; i++) {
            int val = *(const int *)in;
            *out = (unsigned short)val;
            if ((int)(unsigned short)val != val) {
                Com_Error(ERR_DROP, "CMod_LoadLeafBrushes: leaf brush overflows a short");
            }
            in += 4;
            out++;
        }
    }

    {
        const byte *in;
        CollisionAabbTree *out;

        in = bspBase + header->lumps[34].fileofs;
        if (header->lumps[34].filelen & 0x1f) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionAabbTrees: funny lump size");
        }
        count = header->lumps[34].filelen >> 5;

        out = (CollisionAabbTree *)CM_Hunk_Alloc(count * 32, "CMod_LoadCollisionAabbTrees", 0x1a);
        cmLocal->aabbTrees = out;
        cmLocal->aabbTreeCount = count;

        for (i = 0; i < count; i++) {
            out[i].origin[0] = ((const CollisionAabbTree *)in)->origin[0];
            out[i].origin[1] = ((const CollisionAabbTree *)in)->origin[1];
            out[i].origin[2] = ((const CollisionAabbTree *)in)->origin[2];
            out[i].halfSize[0] = ((CollisionAabbTree *)in)->halfSize[0];
            out[i].halfSize[1] = ((CollisionAabbTree *)in)->halfSize[1];
            out[i].halfSize[2] = ((CollisionAabbTree *)in)->halfSize[2];
            out[i].materialIndex = (*(const unsigned short *)&((CollisionAabbTree *)in)->materialIndex);
            out[i].childCount = (*(const unsigned short *)&((CollisionAabbTree *)in)->childCount);
            out[i].u.firstChildIndex = ((CollisionAabbTree *)in)->u.firstChildIndex;
            in += 0x20;
        }
    }

    {
        const lump_t *leafLump;
        const byte *in;
        cLeaf_t *out;
        int leafLumpLen;

        leafLump = &header->lumps[26];

        in = bspBase + leafLump->fileofs;
        leafLumpLen = leafLump->filelen;

        if (leafLumpLen % 36 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadLeafs: funny lump size");
        }
        count = leafLumpLen / 36;
        if (count <= 0) {
            Com_Error(ERR_DROP, "CMod_LoadLeafs: map has no leafs");
        }

        out = (cLeaf_t *)CM_Hunk_Alloc(count * sizeof(cLeaf_t), "CMod_LoadLeafs", 0x17);
        cmLocal->leafs = out;
        cmLocal->numLeafs = count;

        for (i = 0; i < count; i++) {
            if (usePvsFlag) {
                int cluster = ((const dleaf_ondisk_t *)in)->cluster;
                out->cluster = (short)cluster;
                if ((int)(short)cluster != cluster) {
                    Com_Error(ERR_DROP, "CMod_LoadLeafs: cluster overflows a short");
                }

                {
                    int fcaa = ((const dleaf_ondisk_t *)in)->firstCollAabbIndex;
                    out->firstCollAabbIndex = (unsigned short)fcaa;
                    if ((int)(unsigned short)fcaa != fcaa) {
                        Com_Error(ERR_DROP, "CMod_LoadLeafs: firstCollAabbIndex overflows an unsigned short");
                    }
                }

                {
                    int cac = ((const dleaf_ondisk_t *)in)->collAabbCount;
                    out->collAabbCount = (unsigned short)cac;
                    if ((int)(unsigned short)cac != cac) {
                        Com_Error(ERR_DROP, "CMod_LoadLeafs: collAabbCount overflows an unsigned short");
                    }
                }

                if (cluster >= cmLocal->numClusters) {
                    cmLocal->numClusters = cluster + 1;
                }
            } else {
                int fcaa = ((const dleaf_ondisk_t *)in)->firstCollAabbIndex;
                out->firstCollAabbIndex = (unsigned short)fcaa;
                if ((int)(unsigned short)fcaa != fcaa) {
                    Com_Error(ERR_DROP, "CMod_LoadLeafs: firstCollAabbIndex overflows an unsigned short");
                }

                int cac = ((const dleaf_ondisk_t *)in)->collAabbCount;
                out->collAabbCount = (unsigned short)cac;
                if ((int)(unsigned short)cac != cac) {
                    Com_Error(ERR_DROP, "CMod_LoadLeafs: collAabbCount overflows an unsigned short");
                }
            }

            in += 0x24;
            out++;
        }
    }

    {
        const byte *in;
        cmodel_t *out;
        int subLumpLen;

        in = bspBase + header->lumps[35].fileofs;
        subLumpLen = header->lumps[35].filelen;

        if (subLumpLen % 48 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadSubmodels: funny lump size");
        }
        count = subLumpLen / 48;
        if (count <= 0) {
            Com_Error(ERR_DROP, "CMod_LoadSubmodels: map has no submodels");
        }
        if (count > 1023) {
            Com_Error(ERR_DROP, "CMod_LoadSubmodels: too many submodels");
        }

        out = (cmodel_t *)CM_Hunk_Alloc(count * 72, "CMod_LoadSubmodels", 0x18);
        cmLocal->cmodels = out;
        cmLocal->numSubModels = count;

        for (i = 0; i < count; i++) {
            float extent[3];
            float absMin, absMax;

            for (j = 0; j < 3; j++) {
                float minVal = ((const dsubmodel_ondisk_t *)in)->mins[j] - 1.0f;
                float maxVal = ((const dsubmodel_ondisk_t *)in)->maxs[j] + 1.0f;
                out[i].mins[j] = minVal;
                out[i].maxs[j] = maxVal;

                absMin = minVal < 0.0f ? -minVal : minVal;
                absMax = maxVal < 0.0f ? -maxVal : maxVal;
                extent[j] = (absMin - absMax) < 0.0f ? absMin : absMax;
            }

            out[i].radius = sqrtf(extent[0] * extent[0] + extent[1] * extent[1] + extent[2] * extent[2]);

            if (i > 0) {
                int numBrushes = ((const dsubmodel_ondisk_t *)in)->numBrushes;
                out[i].leaf.collAabbCount = (unsigned short)numBrushes;
                if ((int)(unsigned short)numBrushes != numBrushes) {
                    Com_Error(ERR_DROP, "CMod_LoadSubmodels: collAabbCount overflows a short");
                }

                int firstBrush = ((const dsubmodel_ondisk_t *)in)->firstBrush;
                out[i].leaf.firstCollAabbIndex = (unsigned short)firstBrush;
                if ((int)(unsigned short)firstBrush != firstBrush) {
                    Com_Error(ERR_DROP, "CMod_LoadSubmodels: firstCollAabbIndex overflows a short");
                }
            }

            in += 0x30;
        }
    }

    CM_Hunk_CheckTempMemoryClear();
    TempMemoryReset();
    TempMalloc(0);

    {
        cLeafBrushNode_t *tempBase = (cLeafBrushNode_t *)((byte *)TempMalloc(0) - sizeof(cLeafBrushNode_t));

        cmLocal->leafbrushNodes = (cLeafBrushNode_t *)((byte *)tempBase);
    }

    {
        const lump_t *leafDataLump = &header->lumps[26];
        const byte *leafIn;
        cLeaf_t *leaf;
        int numLeafs;

        leafIn = bspBase + leafDataLump->fileofs;
        leaf = cmLocal->leafs;
        numLeafs = cmLocal->numLeafs;

        for (i = 0; i < numLeafs; i++) {
            int numLeafBrushes;
            int indexFirstLeafBrush;
            int brushContents = 0;
            int terrainContents = 0;

            numLeafBrushes = ((const dleafbrush_ondisk_t *)leafIn)->numLeafBrushes;
            indexFirstLeafBrush = ((const dleafbrush_ondisk_t *)leafIn)->indexFirstLeafBrush;

            if (numLeafBrushes > 0) {
                unsigned short *lb = cmLocal->leafbrushes + indexFirstLeafBrush;
                cbrush_t *brushes = cmLocal->brushes;

                for (j = 0; j < numLeafBrushes; j++) {
                    cbrush_t *b = brushes + lb[j];
                    brushContents |= b->contents;
                }
            }
            leaf->brushContents = brushContents;

            {
                int collCount = leaf->collAabbCount;
                if (collCount > 0) {
                    dmaterial_t *materials = cmLocal->materials;
                    CollisionAabbTree *trees = cmLocal->aabbTrees + leaf->firstCollAabbIndex;

                    for (j = 0; j < collCount; j++) {
                        terrainContents |= materials[trees[j].materialIndex].contentFlags;
                    }
                }
            }
            leaf->terrainContents = terrainContents;

            CMod_PartionLeafBrushes(cmLocal->leafbrushes + indexFirstLeafBrush, numLeafBrushes, leaf);

            leafIn += 0x24;
            leaf++;

            cmLocal = cm_ptr;
        }
    }

    {
        const lump_t *submodelLump = &header->lumps[35];
        const byte *submodelIn;
        int numSubModels;

        submodelIn = bspBase + submodelLump->fileofs;
        numSubModels = cmLocal->numSubModels;

        for (i = 1; i < numSubModels; i++) {
            cmodel_t *cmod;
            int numBrushes;
            int firstBrush;
            unsigned short *indexes;
            int contents;
            cLeaf_t *subLeaf;
            int terrainContents;

            cmod = cmLocal->cmodels + i;
            numBrushes = *(const int *)(submodelIn + i * 0x30 + 0x5C - 0x30);

            numBrushes = *(const int *)(submodelIn + i * 0x30 + 0x2C);
            firstBrush = *(const int *)(submodelIn + i * 0x30 + 0x28);

            if (numBrushes <= 0)
                continue;

            indexes = (unsigned short *)CM_Hunk_Alloc(numBrushes * 2, "CMod_LoadSubmodelBrushNodes", 0x18);

            {
                int brushContents = 0;
                for (j = 0; j < numBrushes; j++) {
                    int brushIdx = firstBrush + j;
                    indexes[j] = (unsigned short)brushIdx;
                    if ((int)(unsigned short)brushIdx != brushIdx) {
                        Com_Error(ERR_DROP, "CMod_LoadSubmodelBrushNodes: brush index overflows a short");
                    }
                    brushContents |= cmLocal->brushes[brushIdx].contents;
                }
                cmod->leaf.brushContents = brushContents;
            }

            subLeaf = &cmod->leaf;
            {
                int collCount = subLeaf->collAabbCount;
                terrainContents = 0;
                if (collCount > 0) {
                    dmaterial_t *materials = cmLocal->materials;
                    CollisionAabbTree *trees = cmLocal->aabbTrees + subLeaf->firstCollAabbIndex;

                    for (j = 0; j < collCount; j++) {
                        terrainContents |= materials[trees[j].materialIndex].contentFlags;
                    }
                }
            }
            cmod->leaf.terrainContents = terrainContents;

            CMod_PartionLeafBrushes(indexes, numBrushes, subLeaf);

            cmLocal = cm_ptr;
        }
    }

    {
        cbrush_t *box_brush;

        box_brush = cmLocal->brushes + cmLocal->numBrushes;
        cmLocal->box_brush = box_brush;
        box_brush->numsides = 0;
        box_brush->sides = (cbrushside_t *)0;
        box_brush->contents = -1;

        cmLocal->box_model.leaf.leafBrushNode = -1;
        cmLocal->box_model.leaf.brushContents = 0;
        cmLocal->box_model.leaf.mins[0] = 3.4028234663852886e+38f;
        cmLocal->box_model.leaf.mins[1] = 3.4028234663852886e+38f;
        cmLocal->box_model.leaf.mins[2] = 3.4028234663852886e+38f;
        cmLocal->box_model.leaf.maxs[0] = -3.4028234663852886e+38f;
        cmLocal->box_model.leaf.maxs[1] = -3.4028234663852886e+38f;
        cmLocal->box_model.leaf.maxs[2] = -3.4028234663852886e+38f;

        box_brush->axialMaterialNum[0][0] = (short)0xFFFF;
        box_brush->axialMaterialNum[0][1] = (short)0xFFFF;
        box_brush->axialMaterialNum[0][2] = (short)0xFFFF;
        box_brush->axialMaterialNum[1][0] = (short)0xFFFF;
        box_brush->axialMaterialNum[1][1] = (short)0xFFFF;
        box_brush->axialMaterialNum[1][2] = (short)0xFFFF;
    }

    {
        cLeafBrushNode_t *boxNode;
        int boxNodeIndex;

        boxNode = (cLeafBrushNode_t *)TempMalloc(sizeof(cLeafBrushNode_t));
        boxNode->axis = 0;
        boxNode->contents = 0;
        boxNode->data.children.range = 0;
        boxNode->data.children.childOffset[0] = 0;
        boxNode->data.children.childOffset[1] = 0;

        *(float *)&boxNode->data = -3.4028234663852886e+38f;

        {
            int byteOffset = (int)((byte *)boxNode - (byte *)cmLocal->leafbrushNodes);
            int dwordOffset = byteOffset >> 2;
            int t = dwordOffset * 3;
            t = t + (t << 4);
            t = t + (t << 8);
            t = t + (t << 16);
            cmLocal->box_model.leaf.leafBrushNode = dwordOffset + t * 4;
        }

        boxNode->leafBrushCount = 1;

        boxNode->data.leaf.brushes = cmLocal->leafbrushes + cmLocal->numLeafBrushes;

        cmLocal->leafbrushes[cmLocal->numLeafBrushes] = cmLocal->numBrushes;
    }

    {
        cLeafBrushNode_t *tempNodes;
        int tempNodesByteSize;
        int nodeCount;

        cmLocal->leafbrushNodes = (cLeafBrushNode_t *)((byte *)cmLocal->leafbrushNodes + sizeof(cLeafBrushNode_t));

        {
            byte *tempTop = (byte *)TempMalloc(0);
            int totalBytes = (int)(tempTop - (byte *)cmLocal->leafbrushNodes);
#if defined(__x86_64__) || defined(_M_X64)

            nodeCount = totalBytes / (int)sizeof(cLeafBrushNode_t);
#else
            int byteOffset = totalBytes >> 2;
            int t = byteOffset * 3;
            t = t + (t << 4);
            t = t + (t << 8);
            t = t + (t << 16);
            nodeCount = byteOffset + t * 4;
#endif
        }

        cmLocal->leafbrushNodesCount = nodeCount + 1;

        {
            cLeafBrushNode_t *permNodes;
            permNodes = (cLeafBrushNode_t *)CM_Hunk_Alloc((nodeCount + 1) * sizeof(cLeafBrushNode_t), "CMod_LoadBrushRelated", 0x18);

            {
                int copySize;
#if defined(__x86_64__) || defined(_M_X64)
                copySize = nodeCount * (int)sizeof(cLeafBrushNode_t);
#else
                int t2 = nodeCount;
                t2 = t2 * 5;
                copySize = t2 * 4;
#endif

                memcpy((byte *)permNodes + sizeof(cLeafBrushNode_t), cmLocal->leafbrushNodes, copySize);
            }
            cmLocal->leafbrushNodes = permNodes;
        }

        CM_Hunk_ClearTempMemory();
    }

    {
        const byte *in;
        cNode_t *out;
        int nodeLumpLen;

        in = bspBase + header->lumps[25].fileofs;
        nodeLumpLen = header->lumps[25].filelen;

        if (nodeLumpLen % 36 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadNodes: funny lump size");
        }
        count = nodeLumpLen / 36;
        if (count <= 0) {
            Com_Error(ERR_DROP, "CMod_LoadNodes: map has no nodes");
        }

        out = (cNode_t *)CM_Hunk_Alloc(count * sizeof(cNode_t), "CMod_LoadNodes", 0x17);
        cmLocal->nodes = out;
        cmLocal->numNodes = count;

        for (i = 0; i < count; i++) {
            int planeNum = ((const dnode_ondisk_t *)in)->planeNum;

            if ((unsigned)planeNum >= (unsigned)cml.numPlanes) {
                Com_Error(ERR_DROP, "CMod_LoadNodes: bad planeNum: %i", planeNum);
            }
            out->plane = cml.planes + planeNum;

            for (j = 0; j < 2; j++) {
                int child = ((const dnode_ondisk_t *)in)->children[j];
                out->children[j] = (short)child;
                if ((int)(short)child != child) {
                    Com_Error(ERR_DROP, "CMod_LoadNodes: child overflows a short");
                }
            }

            in += 0x24;
            out++;
        }
    }

    {
        const byte *in;
        int *out;

        in = bspBase + header->lumps[28].fileofs;
        if (header->lumps[28].filelen & 3) {
            Com_Error(ERR_DROP, "CMod_LoadLeafSurfaces: funny lump size");
        }
        count = header->lumps[28].filelen >> 2;

        out = (int *)CM_Hunk_Alloc(count * 4, "CMod_LoadLeafSurfaces", 0x1a);
        cmLocal->leafsurfaces = out;
        cmLocal->numLeafSurfaces = count;

        for (i = 0; i < count; i++) {
            *out = *(const int *)in;
            in += 4;
            out++;
        }
    }

    {
        const byte *in;
        CollisionVertex *out;

        in = bspBase + header->lumps[29].fileofs;
        if (header->lumps[29].filelen & 0xf) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionVerts: funny lump size");
        }
        count = header->lumps[29].filelen >> 4;

        out = (CollisionVertex *)CM_Hunk_Alloc(count * 12, "CMod_LoadCollisionVerts", 0x1a);
        cmLocal->verts = out;
        cmLocal->vertCount = count;

        for (i = 0; i < count; i++) {
            out->xyz[0] = *(const float *)(in + 4);
            out->xyz[1] = *(const float *)(in + 8);
            out->xyz[2] = *(const float *)(in + 12);
            in += 16;
            out++;
        }
    }

    {
        const byte *in;
        CollisionEdge *out;
        int edgeLumpLen;

        in = bspBase + header->lumps[30].fileofs;
        edgeLumpLen = header->lumps[30].filelen;

        if (edgeLumpLen % 56 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionEdges: funny lump size");
        }
        count = edgeLumpLen / 56;

        out = (CollisionEdge *)CM_Hunk_Alloc(count * 48, "CMod_LoadCollisionEdges", 0x1a);
        cmLocal->edges = out;
        cmLocal->edgeCount = count;

        for (i = 0; i < count; i++) {

            ((float *)out)[0] = ((const dcedge_ondisk_t *)in)->v[0];
            ((float *)out)[1] = ((const dcedge_ondisk_t *)in)->v[1];
            ((float *)out)[2] = ((const dcedge_ondisk_t *)in)->v[2];
            ((float *)out)[3] = ((const dcedge_ondisk_t *)in)->v[3];
            ((float *)out)[4] = ((const dcedge_ondisk_t *)in)->v[4];
            ((float *)out)[5] = ((const dcedge_ondisk_t *)in)->v[5];
            ((float *)out)[6] = ((const dcedge_ondisk_t *)in)->v[6];
            ((float *)out)[7] = ((const dcedge_ondisk_t *)in)->v[7];
            ((float *)out)[8] = ((const dcedge_ondisk_t *)in)->v[8];
            ((float *)out)[9] = ((const dcedge_ondisk_t *)in)->v[9];
            ((float *)out)[10] = ((const dcedge_ondisk_t *)in)->v[10];
            ((float *)out)[11] = ((const dcedge_ondisk_t *)in)->v[11];

            {
                float invLen = 1.0f / ((const dcedge_ondisk_t *)in)->invLen;
                out->axis[2][0] *= invLen;
                out->axis[2][1] *= invLen;
                out->axis[2][2] *= invLen;
            }

            in += 0x38;
            out++;
        }
    }

    {
        const byte *in;
        CollisionTriangle *out;
        int triLumpLen;

        in = bspBase + header->lumps[31].fileofs;
        triLumpLen = header->lumps[31].filelen;

        if (triLumpLen % 72 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionTriangles: funny lump size");
        }
        count = triLumpLen / 72;

        out = (CollisionTriangle *)CM_Hunk_Alloc(count * 72, "CMod_LoadCollisionTriangles", 0x1a);
        cmLocal->tris = out;
        cmLocal->triCount = count;

        for (i = 0; i < count; i++) {

            ((float *)out)[0] = ((const dctri_ondisk_t *)in)->v[0];
            ((float *)out)[1] = ((const dctri_ondisk_t *)in)->v[1];
            ((float *)out)[2] = ((const dctri_ondisk_t *)in)->v[2];
            ((float *)out)[3] = ((const dctri_ondisk_t *)in)->v[3];
            ((float *)out)[4] = ((const dctri_ondisk_t *)in)->v[4];
            ((float *)out)[5] = ((const dctri_ondisk_t *)in)->v[5];
            ((float *)out)[6] = ((const dctri_ondisk_t *)in)->v[6];
            ((float *)out)[7] = ((const dctri_ondisk_t *)in)->v[7];
            ((float *)out)[8] = ((const dctri_ondisk_t *)in)->v[8];
            ((float *)out)[9] = ((const dctri_ondisk_t *)in)->v[9];
            ((float *)out)[10] = ((const dctri_ondisk_t *)in)->v[10];
            ((float *)out)[11] = ((const dctri_ondisk_t *)in)->v[11];

            for (j = 0; j < 3; j++) {
                out->edges[j] = ((const dctri_ondisk_t *)in)->edges[j];
                out->verts[j] = ((const dctri_ondisk_t *)in)->verts[j];
            }

            in += 0x48;
            out++;
        }
    }

    {
        const byte *in;
        CollisionBorder *out;
        int borderLumpLen;

        in = bspBase + header->lumps[32].fileofs;
        borderLumpLen = header->lumps[32].filelen;

        if (borderLumpLen % 28 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionBorders: funny lump size");
        }
        count = borderLumpLen / 28;

        out = (CollisionBorder *)CM_Hunk_Alloc(count * 28, "CMod_LoadCollisionBorders", 0x1a);
        cmLocal->borders = out;
        cmLocal->borderCount = count;

        for (i = 0; i < count; i++) {
            ((float *)out)[0] = ((const dcborder_ondisk_t *)in)->v[0];
            ((float *)out)[1] = ((const dcborder_ondisk_t *)in)->v[1];
            ((float *)out)[2] = *(const float *)(in + 8);
            ((float *)out)[3] = ((const dcborder_ondisk_t *)in)->v[3];
            ((float *)out)[4] = ((const dcborder_ondisk_t *)in)->v[4];
            ((float *)out)[5] = ((const dcborder_ondisk_t *)in)->v[5];
            ((float *)out)[6] = ((const dcborder_ondisk_t *)in)->v[6];
            in += 0x1c;
            out++;
        }
    }

    {
        const byte *in;
        CollisionPartition *out;
        int partLumpLen;

        in = bspBase + header->lumps[33].fileofs;
        partLumpLen = header->lumps[33].filelen;

        if (partLumpLen % 12 != 0) {
            Com_Error(ERR_DROP, "CMod_LoadCollisionPartitions: funny lump size");
        }
        count = partLumpLen / 12;

        out = (CollisionPartition *)CM_Hunk_Alloc(count * 12, "CMod_LoadCollisionPartitions", 0x1a);
        cmLocal->partitions = out;
        cmLocal->partitionCount = count;

        for (i = 0; i < count; i++) {
            out->triCount = *(const byte *)(in + 2);
            out->borderCount = *(const byte *)(in + 3);
            {
                int triIndex = *(const int *)(in + 4);
                out->tris = cmLocal->tris + triIndex;
            }
            {
                int borderIndex = *(const int *)(in + 8);
                out->borders = cmLocal->borders + borderIndex;
            }
            in += 12;
            out++;
        }
    }

    if (usePvsFlag) {
        const lump_t *visLump = &header->lumps[36];

        if (visLump->filelen == 0) {

            int visSize = (cmLocal->numClusters + 31) & ~31;
            cmLocal->clusterBytes = visSize;

            cmLocal->visibility = (byte *)CM_Hunk_Alloc(visSize, "CMod_LoadVisibility", 9);
            Com_Memset(cmLocal->visibility, 0xFF, visSize);
        } else {
            const byte *buf = bspBase + visLump->fileofs;
            int visDataLen;

            cmLocal->vised = 1;
            cmLocal->numClusters = *(const int *)buf;
            cmLocal->clusterBytes = *(const int *)(buf + 4);

            visDataLen = visLump->filelen - 8;
            cmLocal->visibility = (byte *)CM_Hunk_Alloc(visDataLen, "CMod_LoadVisibility", 9);
            Com_Memcpy(cmLocal->visibility, buf + 8, visDataLen);
        }
    } else {
        if (header->lumps[36].filelen != 0) {
            Com_Error(ERR_DROP, "CMod_LoadVisibility: vis data present but usePvs is false");
        }
    }

    {
        int entLen = header->lumps[37].filelen;
        int entOfs = header->lumps[37].fileofs;

        cmLocal->numEntityChars = entLen;

        cmLocal->entityString = (char *)CM_Hunk_Alloc(entLen, "CMod_LoadEntityString", 9);
        Com_Memcpy(cmLocal->entityString, bspBase + entOfs, entLen);
    }

    cml.base = (void *)0;
}
