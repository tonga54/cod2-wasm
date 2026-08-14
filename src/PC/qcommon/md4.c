#ifdef COD2_DOWNSTREAM_WASM_CORE_PROBE
#include "cod2_wasm_core_types.h"
#else
#include "common_types.h"
#include "imports.h"
#endif
#include <string.h>

extern void Com_Memcpy(void *dest, const void *src, int count);
extern void Com_Memset(void *dest, int val, int count);

__attribute__((used)) unsigned char PADDING[64] = {
    0x80,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

static void MD4Transform(UINT4 *state, const unsigned char *block);
void MD4Update(MD4_CTX *context, const unsigned char *input, unsigned int inputLen);
void MD4Final(unsigned char *digest, MD4_CTX *context);
unsigned int Com_BlockChecksumKey(void *buffer, int length, int key);
unsigned int Com_BlockChecksum(const void *buffer, int length);

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void Decode(UINT4 *output, const unsigned char *input, unsigned int len)
{
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[i] = ((UINT4)input[j]) |
                    (((UINT4)input[j + 1]) << 8) |
                    (((UINT4)input[j + 2]) << 16) |
                    (((UINT4)input[j + 3]) << 24);
    }
}

static inline __attribute__((always_inline)) void Encode(unsigned char *output, const UINT4 *input, unsigned int len)
{
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (unsigned char)(input[i] & 0xff);
        output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
        output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
        output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
    }
}

#define FF(a, b, c, d, x, s)           \
    {                                  \
        (a) += F((b), (c), (d)) + (x); \
        (a) = ROTATE_LEFT((a), (s));   \
    }

#define GG(a, b, c, d, x, s)                               \
    {                                                      \
        (a) += G((b), (c), (d)) + (x) + (UINT4)0x5a827999; \
        (a) = ROTATE_LEFT((a), (s));                       \
    }

#define HH(a, b, c, d, x, s)                               \
    {                                                      \
        (a) += H((b), (c), (d)) + (x) + (UINT4)0x6ed9eba1; \
        (a) = ROTATE_LEFT((a), (s));                       \
    }

static void MD4Transform(UINT4 *state, const unsigned char *block)
{
    UINT4 a, b, c, d;
    UINT4 x[16];

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];

    Decode(x, block, 64);

    FF(a, b, c, d, x[0], 3);
    FF(d, a, b, c, x[1], 7);
    FF(c, d, a, b, x[2], 11);
    FF(b, c, d, a, x[3], 19);
    FF(a, b, c, d, x[4], 3);
    FF(d, a, b, c, x[5], 7);
    FF(c, d, a, b, x[6], 11);
    FF(b, c, d, a, x[7], 19);
    FF(a, b, c, d, x[8], 3);
    FF(d, a, b, c, x[9], 7);
    FF(c, d, a, b, x[10], 11);
    FF(b, c, d, a, x[11], 19);
    FF(a, b, c, d, x[12], 3);
    FF(d, a, b, c, x[13], 7);
    FF(c, d, a, b, x[14], 11);
    FF(b, c, d, a, x[15], 19);

    GG(a, b, c, d, x[0], 3);
    GG(d, a, b, c, x[4], 5);
    GG(c, d, a, b, x[8], 9);
    GG(b, c, d, a, x[12], 13);
    GG(a, b, c, d, x[1], 3);
    GG(d, a, b, c, x[5], 5);
    GG(c, d, a, b, x[9], 9);
    GG(b, c, d, a, x[13], 13);
    GG(a, b, c, d, x[2], 3);
    GG(d, a, b, c, x[6], 5);
    GG(c, d, a, b, x[10], 9);
    GG(b, c, d, a, x[14], 13);
    GG(a, b, c, d, x[3], 3);
    GG(d, a, b, c, x[7], 5);
    GG(c, d, a, b, x[11], 9);
    GG(b, c, d, a, x[15], 13);

    HH(a, b, c, d, x[0], 3);
    HH(d, a, b, c, x[8], 9);
    HH(c, d, a, b, x[4], 11);
    HH(b, c, d, a, x[12], 15);
    HH(a, b, c, d, x[2], 3);
    HH(d, a, b, c, x[10], 9);
    HH(c, d, a, b, x[6], 11);
    HH(b, c, d, a, x[14], 15);
    HH(a, b, c, d, x[1], 3);
    HH(d, a, b, c, x[9], 9);
    HH(c, d, a, b, x[5], 11);
    HH(b, c, d, a, x[13], 15);
    HH(a, b, c, d, x[3], 3);
    HH(d, a, b, c, x[11], 9);
    HH(c, d, a, b, x[7], 11);
    HH(b, c, d, a, x[15], 15);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    Com_Memset((long int *)x, 0, 64);
}

void MD4Update(MD4_CTX *context, const unsigned char *input, unsigned int inputLen)
{
    unsigned int i, index, partLen;

    index = (unsigned int)((context->count[0] >> 3) & 0x3F);

    if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3)) {
        context->count[1]++;
    }
    context->count[1] += ((UINT4)inputLen >> 29);

    partLen = 64 - index;

    if (inputLen >= partLen) {
        Com_Memcpy((long int *)&context->buffer[index], (const long int *)input, partLen);
        MD4Transform(context->state, context->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64) {
            MD4Transform(context->state, &input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }

    Com_Memcpy((long int *)&context->buffer[index], (const long int *)&input[i], inputLen - i);
}

void MD4Final(unsigned char *digest, MD4_CTX *context)
{
    unsigned char bits[8];
    unsigned int index, padLen;

    Encode(bits, context->count, 8);

    index = (unsigned int)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD4Update(context, PADDING, padLen);

    MD4Update(context, bits, 8);

    Encode(digest, context->state, 16);

    Com_Memset((long int *)context, 0, sizeof(MD4_CTX));
}

unsigned int Com_BlockChecksumKey(void *buffer, int length, int key)
{
    MD4_CTX ctx;
    unsigned int digest[4];

    ctx.count[1] = 0;
    ctx.count[0] = 0;
    ctx.state[0] = 0x67452301;
    ctx.state[1] = 0xefcdab89;
    ctx.state[2] = 0x98badcfe;
    ctx.state[3] = 0x10325476;

    MD4Update(&ctx, (const unsigned char *)&key, 4);
    MD4Update(&ctx, (const unsigned char *)buffer, length);
    MD4Final((unsigned char *)digest, &ctx);

    return digest[0] ^ digest[1] ^ digest[2] ^ digest[3];
}

unsigned int Com_BlockChecksum(const void *buffer, int length)
{
    MD4_CTX ctx;
    unsigned int digest[4];

    ctx.count[1] = 0;
    ctx.count[0] = 0;
    ctx.state[0] = 0x67452301;
    ctx.state[1] = 0xefcdab89;
    ctx.state[2] = 0x98badcfe;
    ctx.state[3] = 0x10325476;

    MD4Update(&ctx, (const unsigned char *)buffer, length);
    MD4Final((unsigned char *)digest, &ctx);

    return digest[0] ^ digest[1] ^ digest[2] ^ digest[3];
}
