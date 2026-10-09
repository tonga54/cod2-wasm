#include "../downstream/wasm/web_pixels.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    unsigned char src[] = {0, 20, 255, 128, 91, 3, 42, 0};
    unsigned char expected[] = {255, 20, 0, 128, 42, 3, 91, 0};
    unsigned char dst[10];
    memset(dst, 0xa5, sizeof(dst));
    web_bgra_to_rgba(dst + 1, src, 2);
    assert(memcmp(dst + 1, expected, 8) == 0);
    assert(dst[0] == 0xa5 && dst[9] == 0xa5);
    web_bgra_to_rgba(src, src, 2);
    assert(memcmp(src, expected, 8) == 0);
    web_bgra_to_rgba(NULL, NULL, 0);
    puts("PASS: BGRA to RGBA upload preserves alpha, bounds and in-place conversion");
}
