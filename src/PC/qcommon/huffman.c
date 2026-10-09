#include "common_types.h"
#include "imports.h"

#define NODE_LEFT(n) ((node_t *)(n)->left)
#define NODE_RIGHT(n) ((node_t *)(n)->right)
#define NODE_PARENT(n) ((node_t *)(n)->parent)
#define NODE_NEXT(n) ((node_t *)(n)->next)
#define NODE_PREV(n) ((node_t *)(n)->prev)
#define NODE_HEAD(n) ((node_t **)(n)->head)

#if defined(__x86_64__) || defined(_M_X64)

#    define NODE_PTR_CAST (intptr_t)  /* NOT (long): long is 4 bytes on Win64 (LLP64) and truncates */
#else
#    define NODE_PTR_CAST (int)
#endif

#define SET_LEFT(n, v) ((n)->left = NODE_PTR_CAST(v))
#define SET_RIGHT(n, v) ((n)->right = NODE_PTR_CAST(v))
#define SET_PARENT(n, v) ((n)->parent = NODE_PTR_CAST(v))
#define SET_NEXT(n, v) ((n)->next = NODE_PTR_CAST(v))
#define SET_PREV(n, v) ((n)->prev = NODE_PTR_CAST(v))
#define SET_HEAD(n, v) ((n)->head = NODE_PTR_CAST(v))

#define NYT 256
#define INTERNAL_NODE 257

static int bloc;

extern void Com_Memset(void *dest, int val, int count);

void Huff_Init(huffman_t *huff);
void Huff_offsetReceive(node_t *node, int *ch, byte *fin, int *offset);
static void increment(huff_t *huff, node_t *node);
void Huff_addRef(huff_t *huff, int ch);
void huffman_send(node_t *node, node_t *child, byte *fout);
void Huff_offsetTransmit(huff_t *huff, int ch, byte *fout, int *offset);

static inline __attribute__((always_inline)) void add_bit(int bit, byte *fout)
{
    if ((bloc & 7) == 0) {
        fout[bloc >> 3] = 0;
    }
    fout[bloc >> 3] |= bit << (bloc & 7);
    bloc++;
}

static inline __attribute__((always_inline)) int get_bit(byte *fin)
{
    int t;
    t = (fin[bloc >> 3] >> (bloc & 7)) & 0x1;
    bloc++;
    return t;
}

static inline __attribute__((always_inline)) node_t **get_ppnode(huff_t *huff)
{
    node_t **tppnode;
    if (!huff->freelist) {
        return &(huff->nodePtrs[huff->blocPtrs++]);
    } else {
        tppnode = huff->freelist;
        huff->freelist = (node_t **)*tppnode;
        return tppnode;
    }
}

static inline __attribute__((always_inline)) void free_ppnode(huff_t *huff, node_t **ppnode)
{
    *ppnode = (node_t *)huff->freelist;
    huff->freelist = ppnode;
}

static inline __attribute__((always_inline)) void swap(huff_t *huff, node_t *node1, node_t *node2)
{
    node_t *par1, *par2;

    par1 = NODE_PARENT(node1);
    par2 = NODE_PARENT(node2);

    if (par1) {
        if (NODE_LEFT(par1) == node1) {
            SET_LEFT(par1, node2);
        } else {
            SET_RIGHT(par1, node2);
        }
    } else {
        huff->tree = node2;
    }

    if (par2) {
        if (NODE_LEFT(par2) == node2) {
            SET_LEFT(par2, node1);
        } else {
            SET_RIGHT(par2, node1);
        }
    } else {
        huff->tree = node1;
    }

    SET_PARENT(node1, par2);
    SET_PARENT(node2, par1);
}

static inline __attribute__((always_inline)) void swaplist(node_t *node1, node_t *node2)
{
    node_t *par1;

    par1 = NODE_NEXT(node1);
    SET_NEXT(node1, NODE_NEXT(node2));
    SET_NEXT(node2, par1);

    par1 = NODE_PREV(node1);
    SET_PREV(node1, NODE_PREV(node2));
    SET_PREV(node2, par1);

    if (NODE_NEXT(node1) == node1) {
        SET_NEXT(node1, node2);
    }
    if (NODE_NEXT(node2) == node2) {
        SET_NEXT(node2, node1);
    }
    if (NODE_NEXT(node1)) {
        SET_PREV(NODE_NEXT(node1), node1);
    }
    if (NODE_NEXT(node2)) {
        SET_PREV(NODE_NEXT(node2), node2);
    }
    if (NODE_PREV(node1)) {
        SET_NEXT(NODE_PREV(node1), node1);
    }
    if (NODE_PREV(node2)) {
        SET_NEXT(NODE_PREV(node2), node2);
    }
}

static void increment(huff_t *huff, node_t *node)
{
    node_t *lnode;

    if (!node) {
        return;
    }

    if (NODE_NEXT(node) != NULL && NODE_NEXT(node)->weight == node->weight) {
        lnode = *NODE_HEAD(node);
        if (lnode != NODE_PARENT(node)) {
            swap(huff, lnode, node);
        }
        swaplist(lnode, node);
    }
    if (NODE_PREV(node) && NODE_PREV(node)->weight == node->weight) {
        *NODE_HEAD(node) = NODE_PREV(node);
    } else {
        *NODE_HEAD(node) = NULL;
        free_ppnode(huff, NODE_HEAD(node));
    }
    node->weight++;
    if (NODE_NEXT(node) && NODE_NEXT(node)->weight == node->weight) {
        SET_HEAD(node, NODE_NEXT(node)->head);
    } else {
        SET_HEAD(node, get_ppnode(huff));
        *NODE_HEAD(node) = node;
    }
    if (NODE_PARENT(node)) {
        increment(huff, NODE_PARENT(node));
        if (NODE_PREV(node) == NODE_PARENT(node)) {
            swaplist(node, NODE_PARENT(node));
            if (*NODE_HEAD(node) == node) {
                *NODE_HEAD(node) = NODE_PARENT(node);
            }
        }
    }
}

void Huff_offsetReceive(node_t *node, int *ch, byte *fin, int *offset)
{
    bloc = *offset;
    while (node && node->symbol == INTERNAL_NODE) {
        if (get_bit(fin)) {
            node = NODE_RIGHT(node);
        } else {
            node = NODE_LEFT(node);
        }
    }
    if (!node) {
        *ch = 0;
        return;
    }
    *ch = node->symbol;
    *offset = bloc;
}

static inline __attribute__((always_inline)) void huffman_send_core(node_t *node, node_t *child, byte *fout)
{
    if (NODE_PARENT(node)) {
        huffman_send(NODE_PARENT(node), node, fout);
    }
    if (child) {
        if (NODE_RIGHT(node) == child) {
            add_bit(1, fout);
        } else {
            add_bit(0, fout);
        }
    }
}

void huffman_send(node_t *node, node_t *child, byte *fout)
{
    huffman_send_core(node, child, fout);
}

void Huff_addRef(huff_t *huff, int ch)
{
    node_t *tnode, *tnode2;

    ch = (byte)ch;

    if (huff->loc[ch] != NULL) {

        increment(huff, huff->loc[ch]);
        return;
    }

    tnode = &(huff->nodeList[huff->blocNode]);
    tnode2 = &(huff->nodeList[huff->blocNode + 1]);
    huff->blocNode += 2;

    tnode2->symbol = INTERNAL_NODE;
    tnode2->weight = 1;
    SET_NEXT(tnode2, NODE_NEXT(huff->lhead));
    if (NODE_NEXT(huff->lhead)) {
        SET_PREV(NODE_NEXT(huff->lhead), tnode2);
        if (NODE_NEXT(huff->lhead)->weight == 1) {
            SET_HEAD(tnode2, NODE_NEXT(huff->lhead)->head);
        } else {
            SET_HEAD(tnode2, get_ppnode(huff));
            *NODE_HEAD(tnode2) = tnode2;
        }
    } else {
        SET_HEAD(tnode2, get_ppnode(huff));
        *NODE_HEAD(tnode2) = tnode2;
    }
    SET_NEXT(huff->lhead, tnode2);
    SET_PREV(tnode2, huff->lhead);

    tnode->symbol = ch;
    tnode->weight = 1;
    SET_NEXT(tnode, NODE_NEXT(huff->lhead));
    if (NODE_NEXT(huff->lhead)) {
        SET_PREV(NODE_NEXT(huff->lhead), tnode);
        if (NODE_NEXT(huff->lhead)->weight == 1) {
            SET_HEAD(tnode, NODE_NEXT(huff->lhead)->head);
        } else {
            SET_HEAD(tnode, get_ppnode(huff));
            *NODE_HEAD(tnode) = tnode;
        }
    } else {
        SET_HEAD(tnode, get_ppnode(huff));
        *NODE_HEAD(tnode) = tnode;
    }
    SET_NEXT(huff->lhead, tnode);
    SET_PREV(tnode, huff->lhead);
    SET_LEFT(tnode, NULL);
    SET_RIGHT(tnode, NULL);

    if (NODE_PARENT(huff->lhead)) {
        if (NODE_LEFT(NODE_PARENT(huff->lhead)) == huff->lhead) {
            SET_LEFT(NODE_PARENT(huff->lhead), tnode2);
        } else {
            SET_RIGHT(NODE_PARENT(huff->lhead), tnode2);
        }
    } else {
        huff->tree = tnode2;
    }

    SET_RIGHT(tnode2, tnode);
    SET_LEFT(tnode2, huff->lhead);

    SET_PARENT(tnode2, NODE_PARENT(huff->lhead));
    SET_PARENT(huff->lhead, tnode2);
    SET_PARENT(tnode, tnode2);

    huff->loc[ch] = tnode;

    increment(huff, NODE_PARENT(tnode2));
}

void Huff_offsetTransmit(huff_t *huff, int ch, byte *fout, int *offset)
{
    bloc = *offset;
    huffman_send_core(huff->loc[ch], NULL, fout);
    *offset = bloc;
}

void Huff_Init(huffman_t *huff)
{
    node_t *node;

    Com_Memset(&huff->compressor, 0, sizeof(huff_t));
    Com_Memset(&huff->decompressor, 0, sizeof(huff_t));

    huff->decompressor.tree = huff->decompressor.lhead = huff->decompressor.ltail =
        huff->decompressor.loc[NYT] = node =
            &(huff->decompressor.nodeList[huff->decompressor.blocNode++]);
    huff->decompressor.tree->symbol = NYT;
    huff->decompressor.tree->weight = 0;
    huff->decompressor.lhead->next = huff->decompressor.lhead->prev = (int)NULL;
    huff->decompressor.tree->parent = huff->decompressor.tree->left =
        huff->decompressor.tree->right = (int)NULL;

    huff->compressor.tree = huff->compressor.lhead = node =
        &(huff->compressor.nodeList[huff->compressor.blocNode++]);
    huff->compressor.tree->symbol = NYT;
    huff->compressor.tree->weight = 0;
    huff->compressor.lhead->next = huff->compressor.lhead->prev = (int)NULL;
    huff->compressor.tree->parent = huff->compressor.tree->left =
        huff->compressor.tree->right = (int)NULL;
    huff->compressor.loc[NYT] = huff->compressor.tree;
}
