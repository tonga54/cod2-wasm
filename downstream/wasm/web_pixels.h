#ifndef COD2_WEB_PIXELS_H
#define COD2_WEB_PIXELS_H
#include <stddef.h>

/* WebGL accepts RGBA bytes, while the D3D surface stores BGRA bytes. */
static inline void web_bgra_to_rgba(unsigned char *dst, const unsigned char *src, size_t pixels)
{
    for (size_t i = 0; i < pixels; ++i) {
        unsigned char b = src[4*i], g = src[4*i+1], r = src[4*i+2], a = src[4*i+3];
        dst[4*i] = r;
        dst[4*i+1] = g;
        dst[4*i+2] = b;
        dst[4*i+3] = a;
    }
}
#endif
