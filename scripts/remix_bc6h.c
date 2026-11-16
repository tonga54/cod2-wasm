/* Optional offline decoder; no native code is loaded by the browser. */
#define BCDEC_IMPLEMENTATION
#include "../third_party/bcdec/bcdec.h"
#include <string.h>

void remix_bc6h(const unsigned char *blocks, float *rgb, int width, int height, int is_signed)
{
    int x, y, row;
    for (y = 0; y < height; y += 4)
        for (x = 0; x < width; x += 4) {
            float tile[48];
            bcdec_bc6h_float(blocks, tile, 12, is_signed);
            blocks += 16;
            for (row = 0; row < 4 && y + row < height; ++row)
                memcpy(rgb + ((y + row) * width + x) * 3, tile + row * 12,
                       (width - x < 4 ? width - x : 4) * 3 * sizeof(float));
        }
}
