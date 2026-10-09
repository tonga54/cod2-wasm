#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern int alwaysfails;
#include "bytematch.h"
extern DxGlobals dx;

COD2_ASSERT_FIELD(DxGlobals, smodelCacheIndices, 0x2dc8);

static static_model_cache_t s_cache;

extern refimport_t ri;
extern GfxBackEndData *frontEndDataOut;

extern r_global_permanent_t rgp;

void R_AddFrontendCmd(int type, void *data);
void R_InitStaticModelIndexCache(void);
void R_StaticModelCacheStats_f(void);
void R_UsedCachedStaticModelSurface(GfxStaticModelSurfaceCached *surf);
void R_SkinStaticModelCachedCmd(SkinStaticModelCachedCmd *skinCmd, SkinBuffers *skinBuffers);
void R_InitStaticModelCache(void);
static __attribute_regparm__(3) void SMC_FreeCachedSurface_r(static_model_cache_t *cache, void *tree, int nodeIndex, int levelsToLeaf);
void R_StaticModelCacheFlush_f(void);
void R_ShutdownStaticModelCache(void);
static __attribute_regparm__(2) Bool SMC_GetFreeBlockOfSize(static_model_cache_t *cache, int listIndex);
GfxStaticModelSurfaceCached *R_CacheStaticModelSurface(GfxStaticSurface *staticSurf, const XSurface *xsurf, int smodelIndex, const Material *material);
void R_FlushStaticModelCache(void);

void R_InitStaticModelIndexCache(void)
{
    void *mem = ((void *(*)(int))(ri.Hunk_AllocInternal))(0xc0000);
    dx.smodelCacheIndices = mem;
}

void R_StaticModelCacheStats_f(void)
{
    refimport_t *pri = (refimport_t *)&ri;
    pri->Printf(0, "%.2f%% of cache is currently allocated.\n",
                (double)((float)s_cache.stats.allocatedVerts * 100.0f * (1.0f / 65536.0f)));
    if (s_cache.stats.allocatedVerts) {
        pri->Printf(0, "%.2f%% allocated cache vertices are used.\n",
                    (double)((float)s_cache.stats.usedVerts * 100.0f / (float)s_cache.stats.allocatedVerts));
    }
}

void R_UsedCachedStaticModelSurface(GfxStaticModelSurfaceCached *surf)
{
    int treeIndex;

    treeIndex = ((unsigned int)((char *)surf - (char *)&s_cache)) / sizeof(static_model_tree_t);
    static_model_tree_t *tree = &s_cache.trees[treeIndex];

    tree->frameCount = frontEndDataOut->frameCount;

    ((static_model_tree_list_t *)tree->usedlist.next)->prev = tree->usedlist.prev;
    ((static_model_tree_list_t *)tree->usedlist.prev)->next = tree->usedlist.next;

    tree->usedlist.prev = (intptr_t)&s_cache.usedlist;
    tree->usedlist.next = s_cache.usedlist.next;
    s_cache.usedlist.next = (intptr_t)&tree->usedlist;
    ((static_model_tree_list_t *)tree->usedlist.next)->prev = (intptr_t)&tree->usedlist;
}

void R_InitStaticModelCache(void)
{
    int size;
    int i;
    void *device;
    void **vtable;
    static_model_node_list_t *freenode;

    void **vbOut;
    volatile int *retryFlag;

    size = (((int *)r_rendererInUse)[2] == 2) ? 0x240000 : 0x400000;

    vbOut = (void **)&dx.smodelCacheVb;
    retryFlag = (volatile int *)&alwaysfails;

    do {
        device = ((DxGlobals *)imp_dx)->device;
        vtable = *(void ***)device;
        /* usage = D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY (0x208). NOT 0x400208: the
         * 0x400000 bit is not a valid D3DUSAGE, and real D3D9 fails the whole
         * CreateVertexBuffer on it (Wine ignores it) -> smodelCacheVb stays NULL
         * -> NULL-deref on the first static-model skin. */
        ((HRESULT(D3DVTCC *)(void *, int, int, int, int, void **, void *))vtable[0x68 / 4])(
            device, size, 0x208, 0, 0, vbOut, NULL);
    } while (*retryFlag != 0);

    memset(&s_cache, 0, sizeof(s_cache));

    s_cache.usedlist.prev = (intptr_t)&s_cache.usedlist;
    s_cache.usedlist.next = (intptr_t)&s_cache.usedlist;

    for (i = 0; i < 5; i++) {
        s_cache.freelist[i].prev = (intptr_t)&s_cache.freelist[i];
        s_cache.freelist[i].next = (intptr_t)&s_cache.freelist[i];
    }

    for (i = 0; i < 128; i++) {
        freenode = &s_cache.trees[i].leafs[0].freenode;
        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;
    }
}

static __attribute_regparm__(3) void SMC_FreeCachedSurface_r(static_model_cache_t *cache, void *tree, int nodeIndex, int levelsToLeaf)
{
    char *t = (char *)tree;

    if (*(short *)(t + nodeIndex * 4 + 0xc) == 0) {
        int leafIdx = (nodeIndex + 1) << levelsToLeaf;
        char *surf = t + leafIdx * 16 - 0x80;
        void *next = *(void **)(surf + 0xc);
        void *prev = *(void **)(surf + 0x8);
        *(void **)next = prev;
        *((void **)prev + 1) = next;
        return;
    }

    *(short *)(t + nodeIndex * 4 + 0xc) = 0;

    if (*(byte *)(t + nodeIndex * 4 + 0xe) != 0) {
        int leafIdx = (nodeIndex + 1) << levelsToLeaf;
        char *surf = t + leafIdx * 16 - 0x78;
        char *header = *(char **)(surf + 0xc);

        {
            int lodLevel = 0;
            char *slot = header;
            while (lodLevel < 4) {
                if (*(void **)(slot + lodLevel * 4) == (void *)surf) {
                    *(void **)(slot + lodLevel * 4) = NULL;
                    break;
                }
                lodLevel++;
            }
        }

        *(int *)((char *)cache + 0xc430) -= (1 << (levelsToLeaf + 5));
        {
            short vertCount = *(short *)(*(char **)(surf + 8) + 2);
            *(int *)((char *)cache + 0xc434) -= vertCount;
        }

        *(byte *)(t + nodeIndex * 4 + 0xe) = 0;
        return;
    }

    SMC_FreeCachedSurface_r(cache, tree, nodeIndex * 2 + 1, levelsToLeaf - 1);
    SMC_FreeCachedSurface_r(cache, tree, nodeIndex * 2 + 2, levelsToLeaf - 1);
}

void R_StaticModelCacheFlush_f(void)
{
    static_model_tree_t *tree;
    static_model_node_list_t *freenode;
    GfxStaticModelSurfaceCached *leaf;
    GfxStaticSurface *surface;
    static_model_tree_list_t *node;
    int lodLevel;
    int i;

    tree = (static_model_tree_t *)s_cache.usedlist.next;
    if (!tree)
        return;

    while ((intptr_t)tree != (intptr_t)&s_cache.usedlist) {
        freenode = &tree->leafs[0].freenode;
        leaf = &tree->leafs[0].surf;

        if (tree->nodes[0].usedVerts == 0) {

            ((static_model_node_list_t *)freenode->next)->prev = freenode->prev;
            ((static_model_node_list_t *)freenode->prev)->next = freenode->next;
        } else {
            tree->nodes[0].usedVerts = 0;

            if (tree->nodes[0].inuse == 0) {

                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 1, 3);
                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 2, 3);
                freenode = &tree->leafs[0].freenode;
            } else {

                surface = leaf->surface;
                lodLevel = 0;
                if (surface->cachedLods[0] != leaf) {
                    for (lodLevel = 1; lodLevel < 4; lodLevel++) {
                        if (surface->cachedLods[lodLevel] == leaf)
                            break;
                    }
                }
                if (lodLevel < 4) {
                    surface->cachedLods[lodLevel] = NULL;
                }

                s_cache.stats.allocatedVerts -= 0x200;
                s_cache.stats.usedVerts -= leaf->xsurf->vertCount;
                tree->nodes[0].inuse = 0;
            }
        }

        node = (static_model_tree_list_t *)s_cache.usedlist.next;
        ((static_model_tree_list_t *)node->next)->prev = node->prev;
        ((static_model_tree_list_t *)node->prev)->next = node->next;

        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;

        tree = (static_model_tree_t *)s_cache.usedlist.next;
    }

    memset(&s_cache, 0, sizeof(s_cache));

    s_cache.usedlist.prev = (intptr_t)&s_cache.usedlist;
    s_cache.usedlist.next = (intptr_t)&s_cache.usedlist;

    for (i = 0; i < 5; i++) {
        s_cache.freelist[i].prev = (intptr_t)&s_cache.freelist[i];
        s_cache.freelist[i].next = (intptr_t)&s_cache.freelist[i];
    }

    for (i = 0; i < 128; i++) {
        freenode = &s_cache.trees[i].leafs[0].freenode;
        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;
    }
}

void R_ShutdownStaticModelCache(void)
{
    static_model_tree_t *tree;
    static_model_node_list_t *freenode;
    GfxStaticModelSurfaceCached *leaf;
    GfxStaticSurface *surface;
    static_model_tree_list_t *node;
    int lodLevel;
    int i;
    void *vb;
    void **vtable;

    tree = (static_model_tree_t *)s_cache.usedlist.next;
    if (!tree)
        goto release_vb;

    while ((intptr_t)tree != (intptr_t)&s_cache.usedlist) {
        freenode = &tree->leafs[0].freenode;
        leaf = &tree->leafs[0].surf;

        if (tree->nodes[0].usedVerts == 0) {

            ((static_model_node_list_t *)freenode->next)->prev = freenode->prev;
            ((static_model_node_list_t *)freenode->prev)->next = freenode->next;
        } else {
            tree->nodes[0].usedVerts = 0;

            if (tree->nodes[0].inuse == 0) {

                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 1, 3);
                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 2, 3);
                freenode = &tree->leafs[0].freenode;
            } else {

                surface = leaf->surface;
                lodLevel = 0;
                if (surface->cachedLods[0] != leaf) {
                    for (lodLevel = 1; lodLevel < 4; lodLevel++) {
                        if (surface->cachedLods[lodLevel] == leaf)
                            break;
                    }
                }
                if (lodLevel < 4) {
                    surface->cachedLods[lodLevel] = NULL;
                }

                s_cache.stats.allocatedVerts -= 0x200;
                s_cache.stats.usedVerts -= leaf->xsurf->vertCount;
                tree->nodes[0].inuse = 0;
            }
        }

        node = (static_model_tree_list_t *)s_cache.usedlist.next;
        ((static_model_tree_list_t *)node->next)->prev = node->prev;
        ((static_model_tree_list_t *)node->prev)->next = node->next;

        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;

        tree = (static_model_tree_t *)s_cache.usedlist.next;
    }

    memset(&s_cache, 0, sizeof(s_cache));

    s_cache.usedlist.prev = (intptr_t)&s_cache.usedlist;
    s_cache.usedlist.next = (intptr_t)&s_cache.usedlist;

    for (i = 0; i < 5; i++) {
        s_cache.freelist[i].prev = (intptr_t)&s_cache.freelist[i];
        s_cache.freelist[i].next = (intptr_t)&s_cache.freelist[i];
    }

    for (i = 0; i < 128; i++) {
        freenode = &s_cache.trees[i].leafs[0].freenode;
        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;
    }

release_vb:

    vb = *&dx.smodelCacheVb;
    if (vb) {
        do {
            vb = *&dx.smodelCacheVb;
            vtable = *(void ***)vb;
            if (vtable && vtable[8 / 4])
                ((int(D3DVTCC *)(void *))vtable[8 / 4])(vb);
            *&dx.smodelCacheVb = NULL;
        } while (*(volatile int *)&alwaysfails != 0);
    }
}

static __attribute_regparm__(2)
    Bool SMC_GetFreeBlockOfSize(static_model_cache_t *cache, int listIndex)
{
    static_model_tree_t *tree;
    static_model_node_list_t *block;
    static_model_node_list_t *freenode;
    static_model_node_list_t *buddyFreenode;
    static_model_node_list_t *list;
    GfxStaticModelSurfaceCached *leaf;
    GfxStaticSurface *surface;
    int parentListIndex;
    int treeIndex;
    int leafIndex, buddyLeafIndex;
    int lodLevel;

    if (listIndex == 0) {

        tree = (static_model_tree_t *)cache->usedlist.prev;

        if (frontEndDataOut->frameCount - tree->frameCount <= 2)
            return (Bool)0;

        freenode = &tree->leafs[0].freenode;
        leaf = &tree->leafs[0].surf;

        if (tree->nodes[0].usedVerts == 0) {

            ((static_model_node_list_t *)freenode->next)->prev = freenode->prev;
            ((static_model_node_list_t *)freenode->prev)->next = freenode->next;
        } else {
            tree->nodes[0].usedVerts = 0;

            if (tree->nodes[0].inuse == 0) {

                SMC_FreeCachedSurface_r(cache, (void *)tree, 1, 3);
                SMC_FreeCachedSurface_r(cache, (void *)tree, 2, 3);
                freenode = &tree->leafs[0].freenode;
            } else {

                surface = leaf->surface;
                lodLevel = 0;
                if (surface->cachedLods[0] != leaf) {
                    for (lodLevel = 1; lodLevel < 4; lodLevel++) {
                        if (surface->cachedLods[lodLevel] == leaf)
                            break;
                    }
                }
                if (lodLevel < 4) {
                    surface->cachedLods[lodLevel] = NULL;
                }

                cache->stats.allocatedVerts -= 0x200;
                cache->stats.usedVerts -= leaf->xsurf->vertCount;
                tree->nodes[0].inuse = 0;
            }
        }

        ((static_model_tree_list_t *)tree->usedlist.next)->prev = tree->usedlist.prev;
        ((static_model_tree_list_t *)tree->usedlist.prev)->next = tree->usedlist.next;

        list = &cache->freelist[0];
        freenode->prev = (intptr_t)list;
        freenode->next = list->next;
        list->next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;

        return (Bool)1;
    }

    parentListIndex = listIndex - 1;

    if (cache->freelist[parentListIndex].next == (intptr_t)&cache->freelist[parentListIndex]) {

        if (!SMC_GetFreeBlockOfSize(cache, parentListIndex))
            return 0;
    }

    block = (static_model_node_list_t *)cache->freelist[parentListIndex].next;

    ((static_model_node_list_t *)block->next)->prev = block->prev;
    ((static_model_node_list_t *)block->prev)->next = block->next;

    treeIndex = ((unsigned int)((char *)block - (char *)cache)) / sizeof(static_model_tree_t);
    tree = &cache->trees[treeIndex];

    if (listIndex == 1) {
        tree->usedlist.prev = (intptr_t)&cache->usedlist;
        tree->usedlist.next = cache->usedlist.next;
        cache->usedlist.next = (intptr_t)&tree->usedlist;
        ((static_model_tree_list_t *)tree->usedlist.next)->prev = (intptr_t)&tree->usedlist;
    }

    list = &cache->freelist[listIndex];
    block->prev = (intptr_t)list;
    block->next = list->next;
    list->next = (intptr_t)block;
    ((static_model_node_list_t *)block->next)->prev = (intptr_t)block;

    leafIndex = (unsigned int)((char *)block - (char *)&tree->leafs[0]) >> 4;
    buddyLeafIndex = leafIndex + (1 << (4 - listIndex));
    buddyFreenode = &tree->leafs[buddyLeafIndex].freenode;

    buddyFreenode->prev = (intptr_t)list;
    buddyFreenode->next = (intptr_t)block;
    list->next = (intptr_t)buddyFreenode;
    ((static_model_node_list_t *)buddyFreenode->next)->prev = (intptr_t)buddyFreenode;

    return 1;
}

GfxStaticModelSurfaceCached *R_CacheStaticModelSurface(GfxStaticSurface *staticSurf, const XSurface *xsurf, int smodelIndex, const Material *material)
{
    GfxStaticModelSurfaceCached *cached;
    static_model_tree_t *tree;
    static_model_node_list_t *block;
    int bitCount, blockSize, listIndex;
    int vertCount;
    int treeIndex, leafIndex, nodeIndex;
    int vertAlloc;
    int baseVertIndex;
    unsigned int twoBaseOffsets;
    int *twoSrcIndices;
    int *twoDstIndices;
    int numTriPairs;
    int i;
    SkinStaticModelCachedCmd skinCmd;

    if (dx.deviceLost != 0)
        return NULL;

    if (xsurf->vertCount > 0x200)
        return NULL;
    vertCount = xsurf->vertCount;

    if (vertCount <= 0x20) {
        bitCount = 5;
        blockSize = 0x20;
        listIndex = 4;
    } else {
        bitCount = 5;
        do {
            bitCount++;
            blockSize = 1 << bitCount;
        } while (vertCount > blockSize);
        listIndex = 9 - bitCount;
    }

    if (s_cache.freelist[listIndex].next == (intptr_t)&s_cache.freelist[listIndex]) {
        if (!SMC_GetFreeBlockOfSize(&s_cache, listIndex))
            return NULL;
    }

    block = (static_model_node_list_t *)s_cache.freelist[listIndex].next;

    ((static_model_node_list_t *)block->next)->prev = block->prev;
    ((static_model_node_list_t *)block->prev)->next = block->next;

    treeIndex = ((unsigned int)((char *)block - (char *)&s_cache)) / sizeof(static_model_tree_t);
    tree = &s_cache.trees[treeIndex];

    if (listIndex == 0) {
        tree->usedlist.prev = (intptr_t)&s_cache.usedlist;
        tree->usedlist.next = s_cache.usedlist.next;
        s_cache.usedlist.next = (intptr_t)&tree->usedlist;
        ((static_model_tree_list_t *)tree->usedlist.next)->prev = (intptr_t)&tree->usedlist;
    }

    leafIndex = (unsigned int)((char *)block - (char *)&tree->leafs[0]) / 16;

    nodeIndex = ((leafIndex + 16) >> (4 - listIndex)) - 1;

#ifdef GFX_REAL_D3D9

    if (getenv("REALD3D9_SMCOVL") && tree->nodes[nodeIndex].inuse) {
        static int ov;
        if (ov++ < 30)
            fprintf(stderr, "[SMCOVL] DOUBLE-ALLOC tree=%d leaf=%d node=%d listIndex=%d smodel=%d vbIdx=%d  <<< overlap bug\n",
                    treeIndex, leafIndex, nodeIndex, listIndex, smodelIndex,
                    (treeIndex * 16 + leafIndex) * 32);
        fflush(stderr);
    }
#endif

    tree->nodes[nodeIndex].inuse = 1;

    vertAlloc = 1 << bitCount;
    while (nodeIndex >= 0) {
        tree->nodes[nodeIndex].usedVerts += vertAlloc;
        nodeIndex = (nodeIndex - 1) >> 1;
    }

    cached = &tree->leafs[leafIndex].surf;
    baseVertIndex = (treeIndex * 16 + leafIndex) * 32;
    cached->baseVertIndex = baseVertIndex;

    s_cache.stats.allocatedVerts += blockSize;
    s_cache.stats.usedVerts += xsurf->vertCount;

    cached->surface = staticSurf;
    cached->xsurf = xsurf;
    cached->smodelIndex = smodelIndex;

    skinCmd.cached = cached;
    skinCmd.smodelIndex = smodelIndex;
    skinCmd.material = material;
    R_AddFrontendCmd(5, &skinCmd);

    twoBaseOffsets = (baseVertIndex << 16) | (baseVertIndex & 0xFFFF);
    twoSrcIndices = (int *)xsurf->triIndices;
    twoDstIndices = (int *)((intptr_t)dx.smodelCacheIndices + baseVertIndex * 12);
    numTriPairs = (short)((xsurf->triCount + (((unsigned short)xsurf->triCount) >> 15)) >> 1);

    for (i = 0; i < numTriPairs; i++) {
        twoDstIndices[0] = twoBaseOffsets + twoSrcIndices[0];
        twoDstIndices[1] = twoBaseOffsets + twoSrcIndices[1];
        twoDstIndices[2] = twoBaseOffsets + twoSrcIndices[2];
        twoDstIndices += 3;
        twoSrcIndices += 3;
    }

    return cached;
}

void R_FlushStaticModelCache(void)
{
    static_model_tree_t *tree;
    static_model_node_list_t *freenode;
    GfxStaticModelSurfaceCached *leaf;
    GfxStaticSurface *surface;
    static_model_tree_list_t *node;
    int lodLevel;
    int i;

    tree = (static_model_tree_t *)s_cache.usedlist.next;
    if (!tree)
        return;

    while ((intptr_t)tree != (intptr_t)&s_cache.usedlist) {
        freenode = &tree->leafs[0].freenode;
        leaf = &tree->leafs[0].surf;

        if (tree->nodes[0].usedVerts == 0) {

            ((static_model_node_list_t *)freenode->next)->prev = freenode->prev;
            ((static_model_node_list_t *)freenode->prev)->next = freenode->next;
        } else {
            tree->nodes[0].usedVerts = 0;

            if (tree->nodes[0].inuse == 0) {

                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 1, 3);
                SMC_FreeCachedSurface_r(&s_cache, (void *)tree, 2, 3);
                freenode = &tree->leafs[0].freenode;
            } else {

                surface = leaf->surface;
                lodLevel = 0;
                if (surface->cachedLods[0] != leaf) {
                    for (lodLevel = 1; lodLevel < 4; lodLevel++) {
                        if (surface->cachedLods[lodLevel] == leaf)
                            break;
                    }
                }
                if (lodLevel < 4) {
                    surface->cachedLods[lodLevel] = NULL;
                }

                s_cache.stats.allocatedVerts -= 0x200;
                s_cache.stats.usedVerts -= (short)leaf->xsurf->vertCount;
                tree->nodes[0].inuse = 0;
            }
        }

        node = (static_model_tree_list_t *)s_cache.usedlist.next;
        ((static_model_tree_list_t *)node->next)->prev = node->prev;
        ((static_model_tree_list_t *)node->prev)->next = node->next;

        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;

        tree = (static_model_tree_t *)s_cache.usedlist.next;
    }

    memset(&s_cache, 0, sizeof(s_cache));

    s_cache.usedlist.prev = (intptr_t)&s_cache.usedlist;
    s_cache.usedlist.next = (intptr_t)&s_cache.usedlist;

    for (i = 0; i < 5; i++) {
        s_cache.freelist[i].prev = (intptr_t)&s_cache.freelist[i];
        s_cache.freelist[i].next = (intptr_t)&s_cache.freelist[i];
    }

    for (i = 0; i < 128; i++) {
        freenode = &s_cache.trees[i].leafs[0].freenode;
        freenode->prev = (intptr_t)&s_cache.freelist[0];
        freenode->next = s_cache.freelist[0].next;
        s_cache.freelist[0].next = (intptr_t)freenode;
        ((static_model_node_list_t *)freenode->next)->prev = (intptr_t)freenode;
    }
}

extern void AxisTransformVector(const void *matrix, float x, float y, float z, vec_t *out);
extern float Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern int RB_DeriveEntityLights(vec4_t *colorForDir, float sunVisibility, const Material *material, D3DLIGHT9 *lights, int maxLights);
extern void R_FatalLockError(HRESULT hr);
extern void *CColorConverter_GetColorConverter(int mode);
extern int XSurfaceGetBoneOffset(void *surface);
extern float floorf(float x);

void R_SkinStaticModelCachedCmd(SkinStaticModelCachedCmd *skinCmd, SkinBuffers *skinBuffers)
{
    byte *cached;
    byte *xsurf;
    byte *smodelInst;
    int smodelIndex;
    int isDx7;
    float boneMatrix[16];
    float useAxis[12];
    float normAxis[9];
    int vertCount;
    int baseVertIndex;
    byte *skinVerts;
    byte *pSrc;
    byte *pDst;
    void *bufferData;
    void *colorConverter;
    short baseLightingCoords[2];
    int i;
    HRESULT hr;

    cached = *(byte **)skinCmd;
    xsurf = *(byte **)(cached + 8);
    smodelIndex = *((int *)skinCmd + 1);

    {
        byte *rgp = (byte *)imp_rgp;
        byte *world = (*(byte **)&((r_global_permanent_t *)rgp)->world);
        smodelInst = (*(byte **)&((GfxWorld *)world)->smodelInsts) + smodelIndex * 96;
    }
#ifdef GFX_REAL_D3D9

    if (getenv("REALD3D9_SMILOG")) {
        static int seen[4096];
        static int budget = 60;
        if (budget > 0 && smodelIndex < 4096 && !seen[smodelIndex]) {
            seen[smodelIndex] = 1;
            budget--;
            float *o = (float *)(smodelInst + 4);
            float *ax = (float *)&((GfxStaticModelInstance *)smodelInst)->axis[0][0];
            float sc = ((GfxStaticModelInstance *)smodelInst)->scale;
            fprintf(stderr, "[SMI] idx=%d origin=(%.0f %.0f %.0f) scale=%.3f ax0=(%.2f %.2f %.2f)\n",
                    smodelIndex, o[0], o[1], o[2], sc, ax[0], ax[1], ax[2]);
            fflush(stderr);
        }
    }
#endif

    {
        byte *ri_ptr = (byte *)&ri;
        void *(*getBoneData)(int, int);
        byte *boneData;
        int boneOffset;

        getBoneData = (void *(*)(int, int))((refimport_t *)ri_ptr)->XModelGetBasePoseBone;
        boneOffset = XSurfaceGetBoneOffset(xsurf);
        boneData = (byte *)getBoneData((*(int *)&((GfxStaticModelInstance *)smodelInst)->model), boneOffset);

        {
            float *q = (float *)boneData;
            float w2 = q[7];
            float x2 = w2 * q[0], y2 = w2 * q[1], z2 = w2 * q[2];
            float xx = x2 * q[0], xy = x2 * q[1], xz = x2 * q[2], xw = x2 * q[3];
            float yy = y2 * q[1], yz = y2 * q[2], yw = y2 * q[3];
            float zz = z2 * q[2], zw = z2 * q[3];

            boneMatrix[0] = 1.0f - (yy + zz);
            boneMatrix[1] = zw + xy;
            boneMatrix[2] = xz - yw;
            boneMatrix[3] = 0;
            boneMatrix[4] = xy - zw;
            boneMatrix[5] = 1.0f - (xx + zz);
            boneMatrix[6] = xw + yz;
            boneMatrix[7] = 0;
            boneMatrix[8] = xz + yw;
            boneMatrix[9] = yz - xw;
            boneMatrix[10] = 1.0f - (xx + yy);
            boneMatrix[11] = 0;

            boneMatrix[12] = q[4];
            boneMatrix[13] = q[5];
            boneMatrix[14] = q[6];
            boneMatrix[15] = 1.0f;
        }
    }

    {
        extern void R_GetRigidTransform(const float *boneMatrix, const float *origin,
                                        const float *axis, float scale, float *outAxis);
        R_GetRigidTransform(boneMatrix, (float *)(smodelInst + 4),
                            (float *)&((GfxStaticModelInstance *)smodelInst)->axis[0][0], ((GfxStaticModelInstance *)smodelInst)->scale, useAxis);
    }

    Vec3NormalizeTo(useAxis + 0, normAxis + 0);
    Vec3NormalizeTo(useAxis + 3, normAxis + 3);
    Vec3NormalizeTo(useAxis + 6, normAxis + 6);

    {
        byte *xsurfPtr = xsurf;
        vertCount = (int)(*(short *)(xsurfPtr + 2));
        baseVertIndex = *(int *)cached;
    }

    skinVerts = (*(byte **)&((XSurface *)xsurf)->verts);

    isDx7 = (r_rendererInUse->current.integer == 2);

    if (!isDx7) {

        pSrc = (byte *)skinBuffers + 0x2000;

        {
            byte *rgp2 = (byte *)imp_rgp;
            byte *world2 = (*(byte **)&((r_global_permanent_t *)rgp2)->world);
            float *blc = (float *)((*(byte **)&((GfxWorld *)world2)->smodelInsts) + smodelIndex * 96 + 0x54);
            float val;

            val = blc[0] * 32768.0f + 0.5f;
            baseLightingCoords[0] = (short)(intptr_t)floorf(val);
            val = blc[1] * 32768.0f + 0.5f;
            baseLightingCoords[1] = (short)(intptr_t)floorf(val);
        }

        if (vertCount > 0) {
            byte *src = pSrc;
            byte *dst = pSrc;

            for (i = 0; i < vertCount; i++) {
                float *srcPos = (float *)(skinVerts + i * 0x40 + 0x30);
                float *dstVert = (float *)(src + i * 0x40);

                dstVert[0] = srcPos[0] * useAxis[0] + srcPos[1] * useAxis[3] + srcPos[2] * useAxis[6] + useAxis[9];
                dstVert[1] = srcPos[0] * useAxis[1] + srcPos[1] * useAxis[4] + srcPos[2] * useAxis[7] + useAxis[10];
                dstVert[2] = srcPos[0] * useAxis[2] + srcPos[1] * useAxis[5] + srcPos[2] * useAxis[8] + useAxis[11];

                {
                    float nx = *(float *)(skinVerts + i * 0x40 + 0);
                    float ny = *(float *)(skinVerts + i * 0x40 + 4);
                    float nz = *(float *)(skinVerts + i * 0x40 + 8);
                    float *dstNorm = (float *)(dst + i * 0x40 + 0xc);
                    AxisTransformVector(normAxis, nx, ny, nz, dstNorm);
                }

                {
                    byte *srcColor = skinVerts + i * 0x40 + 0xc;
                    byte *dstColor = dst + i * 0x40 + 0x18;
                    dstColor[0] = srcColor[3];
                    dstColor[1] = srcColor[0];
                    dstColor[2] = srcColor[1];
                    dstColor[3] = srcColor[2];
                }

                *(int *)(dst + i * 0x40 + 0x1c) = *(int *)(skinVerts + i * 0x40 + 0x1c);
                *(int *)(dst + i * 0x40 + 0x20) = *(int *)(skinVerts + i * 0x40 + 0x2c);

                *(short *)(dst + i * 0x40 + 0x24) = baseLightingCoords[0];
                *(short *)(dst + i * 0x40 + 0x26) = baseLightingCoords[1];

                {
                    float tx = *(float *)(skinVerts + i * 0x40 + 0x10);
                    float ty = *(float *)(skinVerts + i * 0x40 + 0x14);
                    float tz = *(float *)(skinVerts + i * 0x40 + 0x18);
                    AxisTransformVector(normAxis, tx, ty, tz, (float *)(dst + i * 0x40 + 0x28));
                }

                {
                    float bx = *(float *)(skinVerts + i * 0x40 + 0x20);
                    float by = *(float *)(skinVerts + i * 0x40 + 0x24);
                    float bz = *(float *)(skinVerts + i * 0x40 + 0x28);
                    AxisTransformVector(normAxis, bx, by, bz, (float *)(dst + i * 0x40 + 0x34));
                }
            }
        }

        {
            byte *dx = (byte *)imp_dx;
            byte *vb = (*(byte **)&((DxGlobals *)dx)->smodelCacheVb);
            void **vtable = *(void ***)vb;
            int lockSize = vertCount * 0x40;
            int lockOffset = baseVertIndex * 0x40;

            hr = ((int(D3DVTCC *)(void *, int, int, void **, int))vtable[0x2c / 4])(
                vb, lockOffset, lockSize, &bufferData, 0x1001);
            if (hr < 0) {
                R_FatalLockError(hr);
            }

            colorConverter = CColorConverter_GetColorConverter(0);

            if (vertCount > 0) {
                byte *src2 = pSrc;
                byte *dst2 = (byte *)bufferData;

                for (i = 0; i < vertCount; i++) {

                    memcpy(dst2, src2, 12);

                    {
                        void **ccvt = *(void ***)colorConverter;
                        ((void(D3DVTCC *)(void *, void *, void *))ccvt[0])(colorConverter,
                                                                           dst2 + 0x18, src2 + 0x18);
                    }

                    memcpy(dst2 + 0xc, src2 + 0xc, 8);
                    memcpy(dst2 + 0x10, src2 + 0x10, 8);

                    memcpy(dst2 + 0x1c, src2 + 0x1c, 8);
                    memcpy(dst2 + 0x24, src2 + 0x24, 4);
                    memcpy(dst2 + 0x26, src2 + 0x26, 2);
                    memcpy(dst2 + 0x28, src2 + 0x28, 12);
                    memcpy(dst2 + 0x34, src2 + 0x34, 12);

                    src2 += 0x40;
                    dst2 += 0x40;
                }
            }

            do {
                vtable = *(void ***)vb;
                ((HRESULT(D3DVTCC *)(void *))vtable[0x30 / 4])(vb);
            } while (*(volatile int *)&alwaysfails != 0);
        }
    } else {

        D3DLIGHT9 lights[8];
        int lightCount;

        pSrc = (byte *)skinBuffers + 0x2000;

        {
            byte *rgp3 = (byte *)imp_rgp;
            byte *world3 = (*(byte **)&((r_global_permanent_t *)rgp3)->world);
            float *sunVisPtr = ((GfxWorld *)world3)->smodelLightingSunVisTable;
            byte *lightingColors = (*(byte **)&((GfxWorld *)world3)->smodelLightingColorTable);

            lightCount = RB_DeriveEntityLights(
                (vec4_t *)(lightingColors + smodelIndex * 96),
                sunVisPtr[smodelIndex],
                skinCmd->material, lights, 8);
        }

        if (vertCount > 0) {
            byte *pSrcDx7 = (byte *)skinBuffers + 0x200c;
            byte *srcVert = skinVerts + 0xc;

            for (i = 0; i < vertCount; i++) {
                float *srcPos = (float *)(skinVerts + i * 0x40 + 0x30);
                float normal[3];
                float dstPos[3];

                dstPos[0] = srcPos[0] * useAxis[0] + srcPos[1] * useAxis[3] + srcPos[2] * useAxis[6] + useAxis[9];
                dstPos[1] = srcPos[0] * useAxis[1] + srcPos[1] * useAxis[4] + srcPos[2] * useAxis[7] + useAxis[10];
                dstPos[2] = srcPos[0] * useAxis[2] + srcPos[1] * useAxis[5] + srcPos[2] * useAxis[8] + useAxis[11];

                *(float *)(pSrcDx7 + i * 0x18 - 0xc) = dstPos[0];
                *(float *)(pSrcDx7 + i * 0x18 - 8) = dstPos[1];
                *(float *)(pSrcDx7 + i * 0x18 - 4) = dstPos[2];

                {
                    float nx = *(float *)(skinVerts + i * 0x40 + 0);
                    float ny = *(float *)(skinVerts + i * 0x40 + 4);
                    float nz = *(float *)(skinVerts + i * 0x40 + 8);
                    AxisTransformVector(normAxis, nx, ny, nz, normal);
                }

                *(int *)(pSrcDx7 + i * 0x18 + 4) = *(int *)(skinVerts + i * 0x40 + 0x1c);
                *(int *)(pSrcDx7 + i * 0x18 + 8) = *(int *)(skinVerts + i * 0x40 + 0x2c);

                {
                    float r = 0.0f, g = 0.0f, b = 0.0f;
                    int li;

                    for (li = 0; li < lightCount; li++) {
                        const D3DLIGHT9 *light = &lights[li];
                        float dot;

                        r += light->Ambient.r;
                        g += light->Ambient.g;
                        b += light->Ambient.b;

                        dot = normal[0] * light->Direction.x +
                              normal[1] * light->Direction.y +
                              normal[2] * light->Direction.z;

                        dot = -dot;
                        if (dot > 0.0f) {
                            r += dot * light->Diffuse.r;
                            g += dot * light->Diffuse.g;
                            b += dot * light->Diffuse.b;
                        }
                    }

                    {
                        byte *srcColor = skinVerts + i * 0x40 + 0xc;
                        byte *dstColor = pSrcDx7 + i * 0x18;
                        float oneOver255 = 0.003921568859368563f;
                        int ch;

                        /* Original model colors are RGBA. The legacy converter
                         * below reverses BGRA into ARGB for the vertex buffer. */
                        ch = (intptr_t)floorf(b * (float)srcColor[2] * oneOver255 * 255.0f + 0.5f);
                        if (ch > 255)
                            ch = 255;
                        if (ch < 0)
                            ch = 0;
                        dstColor[0] = (byte)ch;

                        ch = (intptr_t)floorf(g * (float)srcColor[1] * oneOver255 * 255.0f + 0.5f);
                        if (ch > 255)
                            ch = 255;
                        if (ch < 0)
                            ch = 0;
                        dstColor[1] = (byte)ch;

                        ch = (intptr_t)floorf(r * (float)srcColor[0] * oneOver255 * 255.0f + 0.5f);
                        if (ch > 255)
                            ch = 255;
                        if (ch < 0)
                            ch = 0;
                        dstColor[2] = (byte)ch;

                        ch = (intptr_t)floorf((float)srcColor[3] * oneOver255 * 255.0f + 0.5f);
                        if (ch > 255)
                            ch = 255;
                        if (ch < 0)
                            ch = 0;
                        dstColor[3] = (byte)ch;
                    }
                }
            }
        }

        {
            byte *dx = (byte *)imp_dx;
            byte *vb = (*(byte **)&((DxGlobals *)dx)->smodelCacheVb);
            void **vtable = *(void ***)vb;
            int lockSize = vertCount * 0x18;
            int lockOffset = baseVertIndex * 0x18;

            hr = ((int(D3DVTCC *)(void *, int, int, void **, int))vtable[0x2c / 4])(
                vb, lockOffset, lockSize, &bufferData, 0x1001);
            if (hr < 0) {
                R_FatalLockError(hr);
            }

            pSrc = (byte *)skinBuffers + 0x2000;
            pDst = (byte *)bufferData;
            colorConverter = CColorConverter_GetColorConverter(0);

            if (vertCount > 0) {
                for (i = 0; i < vertCount; i++) {

                    memcpy(pDst, pSrc, 12);

                    {
                        void **ccvt = *(void ***)colorConverter;
                        ((void(D3DVTCC *)(void *, void *, void *))ccvt[0])(colorConverter,
                                                                           pDst + 0xc, pSrc + 0xc);
                    }

                    memcpy(pDst + 0x10, pSrc + 0x10, 8);
                    pSrc += 0x18;
                    pDst += 0x18;
                }
            }

            do {
                vtable = *(void ***)vb;
                ((HRESULT(D3DVTCC *)(void *))vtable[0x30 / 4])(vb);
            } while (*(volatile int *)&alwaysfails != 0);
        }
    }
}
