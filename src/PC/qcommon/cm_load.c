#include "common_types.h"
#include "imports.h"
#include <string.h>

extern int stricmp(const char *s1, const char *s2);
extern void CM_LoadMapFromBsp(const char *name, int flag);
extern void CM_LoadStaticModels(void);
extern void CM_ClearWorldTraceCache(void);
extern void *Hunk_AllocInternal(int size);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Memset(void *dest, int val, int count);
extern void *CM_ClipHandleToModel(clipHandle_t handle);
extern int FS_FOpenFileRead(const char *filename, int *file, int uniqueFile);
extern int FS_Read(void *buffer, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern int FS_Write(const void *buffer, int len, int h);
extern int FS_OpenFileOverwrite(const char *filename);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern int FS_ReadFile(const char *filename, void **buffer);
extern int Com_BlockChecksum(const void *buffer, int length);
extern void FS_FreeFile(void *buffer);
extern void *Hunk_AllocateTempMemoryHighInternal(int size);
extern void Hunk_ClearTempMemory(void);
extern void Hunk_ClearTempMemoryHigh(void);

extern byte g_traceThreadInfo[];
extern void *imp_g_traceThreadInfo;

extern struct clipMap_t cm;

void CM_LoadMap(const char *name, int *checksum);
void CM_Shutdown(void);
int CM_NumInlineModels(void);
const char *CM_EntityString(void);
int CM_LeafCluster(int leafnum);
void *CM_Hunk_Alloc(int size, const char *name, int type);
void CM_Hunk_CheckTempMemoryClear(void);
void CM_Hunk_CheckTempMemoryHighClear(void);
void *CM_Hunk_AllocateTempMemoryHigh(int size, const char *name);
void CM_Hunk_ClearTempMemory(void);
void CM_Hunk_ClearTempMemoryHigh(void);
void CM_ModelBounds(clipHandle_t model, vec_t *mins, vec_t *maxs);
void CM_SaveLump(int lumpnum, byte *newLump, int size, int *checksum);

void CM_LoadMap(const char *name, int *checksum)
{
    clipMap_t *cm_base = &cm;
    TraceThreadInfo *phys;
    void *alloc;

    if (name == NULL || *name == '\0') {
        Com_Error(1, "CM_LoadMap: NULL name");
    }

    if (cm_base->name != NULL) {
        if (stricmp(cm_base->name, name) == 0) {
            *checksum = cm_base->checksum;
            return;
        }
    }

    CM_ClearWorldTraceCache();
    CM_LoadMapFromBsp(name, 1);
    CM_LoadStaticModels();

    phys = (TraceThreadInfo *)imp_g_traceThreadInfo;
    phys->checkcount.global = 0;

    alloc = Hunk_AllocInternal(cm_base->partitionCount * 2);
    phys->checkcount.partitions = (unsigned short *)alloc;

    alloc = Hunk_AllocInternal(cm_base->edgeCount * 4);
    phys->checkcount.edges = (int *)alloc;

    alloc = Hunk_AllocInternal(cm_base->vertCount * 4);
    phys->checkcount.verts = (int *)alloc;

    alloc = Hunk_AllocInternal(0x30);
    phys->box_brush = (cbrush_t *)alloc;
    memcpy(alloc, cm_base->box_brush, sizeof(cbrush_t));

    alloc = Hunk_AllocInternal(0x48);
    phys->box_model = (cmodel_t *)alloc;
    memcpy(alloc, &cm_base->box_model, sizeof(cmodel_t));

    *checksum = cm_base->checksum;
}

void CM_Shutdown(void)
{
    CM_ClearWorldTraceCache();
    Com_Memset(&cm, 0, 0x110);
}

int CM_NumInlineModels(void)
{
    return cm.numSubModels;
}

const char *CM_EntityString(void)
{
    return cm.entityString;
}

int CM_LeafCluster(int leafnum)
{
    cLeaf_t *leaves = cm.leafs;
    return leaves[leafnum].cluster;
}

void *CM_Hunk_Alloc(int size, const char *name, int type)
{
    return Hunk_AllocInternal(size);
}

void CM_Hunk_CheckTempMemoryClear(void)
{
}

void CM_Hunk_CheckTempMemoryHighClear(void)
{
}

void *CM_Hunk_AllocateTempMemoryHigh(int size, const char *name)
{
    return Hunk_AllocateTempMemoryHighInternal(size);
}

void CM_Hunk_ClearTempMemory(void)
{
    Hunk_ClearTempMemory();
}

void CM_Hunk_ClearTempMemoryHigh(void)
{
    Hunk_ClearTempMemoryHigh();
}

void CM_ModelBounds(clipHandle_t model, vec_t *mins, vec_t *maxs)
{
    cmodel_t *cmod;

    cmod = CM_ClipHandleToModel(model);

    *(int *)&mins[0] = *(int *)&cmod->mins[0];
    *(int *)&mins[1] = *(int *)&cmod->mins[1];
    *(int *)&mins[2] = *(int *)&cmod->mins[2];

    *(int *)&maxs[0] = *(int *)&cmod->maxs[0];
    *(int *)&maxs[1] = *(int *)&cmod->maxs[1];
    *(int *)&maxs[2] = *(int *)&cmod->maxs[2];
}

void CM_SaveLump(int lumpnum, byte *newLump, int size, int *checksum)
{
    int h;
    byte *buf;
    int length;
    byte header[0x140];
    byte oldHeader[0x140];
    int zero;
    int offset;
    int i;
    int lumpSize;
    int padding;
    byte *data;

    length = FS_FOpenFileRead(*(char **)&cm, &h, 0);
    if (h == 0) {
        Com_Error(1, "EXE_ERR_COULDNT_LOAD%s", *(char **)&cm);
    }

    buf = (byte *)Z_MallocInternal(length + 1);
    FS_Read(buf, length, h);
    buf[length] = '\0';
    FS_FCloseFile(h);

    memcpy(header, buf, sizeof(dheader_t));

    dheader_s *hdr = (dheader_s *)header;
    dheader_s *oldHdr = (dheader_s *)oldHeader;
    lump_t *lumps = (lump_t *)hdr->lumps;
    lump_t *oldLumps = (lump_t *)oldHdr->lumps;

    if (hdr->version != 4 && hdr->version != 0x3d) {
        Com_Error(1, "bad bsp version %d", hdr->version);
    }

    memcpy(oldHeader, header, sizeof(dheader_t));

    h = FS_OpenFileOverwrite(*(char **)&cm);
    if (h == 0) {
        Com_Error(1, "Failed to open file %s for writing", *(char **)&cm);
    }

    offset = 0x140;
    for (i = 0; i < 39; i++) {
        if (i == lumpnum) {
            lumps[i].filelen = size;
        }
        lumps[i].fileofs = offset;
        lumpSize = lumps[i].filelen;
        offset += (lumpSize + 3) & ~3;
    }

    FS_Write(header, 0x140, h);

    zero = 0;
    for (i = 0; i < 39; i++) {
        lumpSize = lumps[i].filelen;
        if (lumpSize == 0)
            continue;

        if (i == lumpnum) {
            data = newLump;
        } else {
            data = buf + oldLumps[i].fileofs;
        }

        FS_Write(data, lumpSize, h);

        padding = ((lumpSize + 3) & ~3) - lumpSize;
        if (padding > 0) {
            FS_Write(&zero, padding, h);
        }
    }

    FS_FCloseFile(h);
    Z_FreeInternal(buf);

    if (checksum != NULL) {
        int fileLen;
        fileLen = FS_ReadFile(*(char **)&cm, (void **)&buf);
        *checksum = Com_BlockChecksum(buf, fileLen);
        FS_FreeFile(buf);
    }
}
