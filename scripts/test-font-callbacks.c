#include "common_types.h"
#include "headers/PC/gfx_d3d/gfx_funcs.h"

/* WebAssembly checks callback return types even when the caller ignores them. */
typedef void (*TextDrawCallback)(const char *, int, FontHandle, float, float,
                                 float, float, const vec_t *, int);
typedef void (*ConsoleDrawCallback)(const short *, int, FontHandle, float, float,
                                    float, float, const vec_t *, int);
_Static_assert(__builtin_types_compatible_p(__typeof__(&R_DrawText), TextDrawCallback),
    "Screen drawing expects a void text callback");
_Static_assert(__builtin_types_compatible_p(__typeof__(&R_DrawConsoleText), ConsoleDrawCallback),
    "Screen drawing expects a void console callback");
_Static_assert(__builtin_types_compatible_p(
    __typeof__(((refexport_t *)0)->DrawText), __typeof__(&R_DrawText)),
    "DrawText callback must match its implementation declaration");
_Static_assert(__builtin_types_compatible_p(
    __typeof__(((refexport_t *)0)->DrawConsoleText), __typeof__(&R_DrawConsoleText)),
    "DrawConsoleText callback must match its implementation declaration");

int main(void) { return 0; }
