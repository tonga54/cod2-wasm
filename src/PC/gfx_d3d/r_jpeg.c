#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"

extern jpeg_error_mgr *jpeg_std_error(jpeg_error_mgr *jerr);
extern void jpeg_CreateCompress(j_compress_ptr cinfo, int version, size_t structsize);
extern void jpeg_set_defaults(j_compress_ptr cinfo);
extern void jpeg_set_quality(j_compress_ptr cinfo, int quality, int force_baseline);
extern void jpeg_start_compress(j_compress_ptr cinfo, int write_all_tables);
extern unsigned int jpeg_write_scanlines(j_compress_ptr cinfo, byte **scanlines, unsigned int num_lines);
extern void jpeg_finish_compress(j_compress_ptr cinfo);
extern void jpeg_destroy_compress(j_compress_ptr cinfo);
extern void jpeg_CreateDecompress(void *cinfo, int version, int structsize);
extern void jpeg_memory_src(void *cinfo, byte *data, int size);
#if COD2_FEATURE_MODERN_LIBS

extern void jpeg_mem_src(void *cinfo, const unsigned char *data, unsigned long size);
#endif
extern int jpeg_read_header(void *cinfo, int require_image);
extern boolean jpeg_start_decompress(void *cinfo);
extern unsigned int jpeg_read_scanlines(void *cinfo, byte **scanlines, unsigned int num_lines);
extern boolean jpeg_finish_decompress(void *cinfo);
extern void jpeg_destroy_decompress(void *cinfo);

extern vidConfig_t vidConfig;
extern refimport_t ri;

static int hackSize;

#define JPEG_COMPRESS_STRUCT_SIZE 372
#define JPEG_C_ERR 0x00
#define JPEG_C_DEST 0x18
#define JPEG_C_IMAGE_WIDTH 0x1c
#define JPEG_C_IMAGE_HEIGHT 0x20
#define JPEG_C_INPUT_COMPONENTS 0x24
#define JPEG_C_IN_COLOR_SPACE 0x28
#define JPEG_C_NEXT_SCANLINE 0xdc

#if COD2_FEATURE_MODERN_LIBS
#    define JPEG_D_STRUCT_SIZE 0x1d0
#    define JPEG_D_OUTPUT_WIDTH 0x70
#    define JPEG_D_OUTPUT_HEIGHT 0x74
#    define JPEG_D_OUT_COLOR_COMPS 0x78
#    define JPEG_D_OUTPUT_SCANLINE 0x8c
#else
#    define JPEG_D_STRUCT_SIZE 0x1b4
#    define JPEG_D_OUTPUT_WIDTH 0x68
#    define JPEG_D_OUTPUT_HEIGHT 0x6c
#    define JPEG_D_OUT_COLOR_COMPS 0x74
#    define JPEG_D_OUTPUT_SCANLINE 0x84
#endif

typedef struct jpeg_memory_destination_s {
    jpeg_destination_mgr pub;
    byte *buffer;
    size_t bufsize;
} jpeg_memory_destination_t;

static void init_destination(j_compress_ptr cinfo);
boolean empty_output_buffer(j_compress_ptr cinfo);
static void term_destination(j_compress_ptr cinfo);
static void *Z_MallocJpeg(size_t size);
static void Z_FreeJpeg(void *ptr, size_t size);
static void ExitJpeg(void);
static void PrintfJpeg(char *message);
void R_SaveJpg(const char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer);
void R_LoadJpg(const char *filepath, byte **file, byte **pic, int *width, int *height, D3DFORMAT *imageFormat);

static void init_destination(j_compress_ptr cinfo)
{
    byte *c = (byte *)cinfo;
    jpeg_memory_destination_t *dest = *(jpeg_memory_destination_t **)(c + JPEG_C_DEST);

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = dest->bufsize;
}

boolean empty_output_buffer(j_compress_ptr cinfo)
{
    (void)cinfo;
    ri.Printf(0, "R_SaveJpg: output buffer overflow\n");
    return 0;
}

static void term_destination(j_compress_ptr cinfo)
{
    byte *c = (byte *)cinfo;
    jpeg_memory_destination_t *dest = *(jpeg_memory_destination_t **)(c + JPEG_C_DEST);

    hackSize = (int)(dest->bufsize - dest->pub.free_in_buffer);
}

static void *Z_MallocJpeg(size_t size)
{
    byte *sys = (byte *)&ri;
    void *(*mallocFunc)(size_t) = *(void *(**)(size_t))(sys + 0x14);
    return mallocFunc(size);
}

static void Z_FreeJpeg(void *ptr, size_t size)
{
    byte *sys = (byte *)&ri;
    void (*freeFunc)(void *, size_t) = *(void (**)(void *, size_t))(sys + 0x18);
    freeFunc(ptr, size);
}

static void ExitJpeg(void)
{
    byte *sys = (byte *)&ri;
    void (*errFunc)(int, const char *) = *(void (**)(int, const char *))(sys + 4);
    errFunc(0, "jpeg internal error");
}

static void PrintfJpeg(char *message)
{
    byte *sys = (byte *)&ri;
    void (*printFunc)(int, const char *, ...) = *(void (**)(int, const char *, ...))(sys);
    printFunc(0, "%s\n", message);
}

void R_SaveJpg(const char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer)
{
    byte cinfo_buf[JPEG_COMPRESS_STRUCT_SIZE];
    jpeg_error_mgr jerr;
    jpeg_memory_destination_t dest;
    byte *cinfo = cinfo_buf;
    byte *out;
    int bufSize;
    byte *row_pointer;

    memset(cinfo, 0, sizeof(cinfo_buf));
    memset(&dest, 0, sizeof(dest));

    *(jpeg_error_mgr **)(cinfo + JPEG_C_ERR) = jpeg_std_error(&jerr);
    jpeg_CreateCompress((j_compress_ptr)cinfo, 0x3e, JPEG_COMPRESS_STRUCT_SIZE);

    bufSize = image_width * image_height * 3;
    out = (byte *)ri.Hunk_AllocateTempMemoryInternal(bufSize);

    dest.pub.init_destination = init_destination;
    dest.pub.empty_output_buffer = empty_output_buffer;
    dest.pub.term_destination = term_destination;
    dest.buffer = out;
    dest.bufsize = (size_t)bufSize;
    *(jpeg_destination_mgr **)(cinfo + JPEG_C_DEST) = &dest.pub;

    *(int *)(cinfo + JPEG_C_IMAGE_WIDTH) = image_width;
    *(int *)(cinfo + JPEG_C_IMAGE_HEIGHT) = image_height;
    *(int *)(cinfo + JPEG_C_INPUT_COMPONENTS) = 3;
    *(int *)(cinfo + JPEG_C_IN_COLOR_SPACE) = 2;

    jpeg_set_defaults((j_compress_ptr)cinfo);
    jpeg_set_quality((j_compress_ptr)cinfo, quality, 1);
    jpeg_start_compress((j_compress_ptr)cinfo, 1);

    {
        int row_stride = image_width * 3;

        if (*(unsigned int *)(cinfo + JPEG_C_NEXT_SCANLINE) < *(unsigned int *)(cinfo + JPEG_C_IMAGE_HEIGHT)) {
            do {
                row_pointer = image_buffer + *(unsigned int *)(cinfo + JPEG_C_NEXT_SCANLINE) * row_stride;
                jpeg_write_scanlines((j_compress_ptr)cinfo, &row_pointer, 1);
            } while (*(unsigned int *)(cinfo + JPEG_C_NEXT_SCANLINE) < *(unsigned int *)(cinfo + JPEG_C_IMAGE_HEIGHT));
        }

        jpeg_finish_compress((j_compress_ptr)cinfo);

        ri.FS_WriteFile(filename, out, hackSize);
        ri.Hunk_FreeTempMemory(out);

        jpeg_destroy_compress((j_compress_ptr)cinfo);
    }
}

void R_LoadJpg(const char *filepath, byte **file, byte **pic, int *width, int *height, D3DFORMAT *imageFormat)
{
    byte cinfo_buf[JPEG_D_STRUCT_SIZE + 0x48];
    byte jerr[0xa8];
    byte *cinfo = cinfo_buf;
    byte *fbuffer;
    int filesize;
    byte *sys;
    byte *limits;
    int maxSize;
    byte *buf;
    int row_stride;
    int output_width;
    int output_height;
    int num_components;
    byte *row_ptr;
    int x;

    sys = (byte *)&ri;
    filesize = ((int (*)(const char *, byte **)) * (void **)(sys + 0x128))(filepath, &fbuffer);

    if (fbuffer == NULL)
        return;

    jpeg_std_error((jpeg_error_mgr *)jerr);
    *(void **)(cinfo) = jerr;
#if !COD2_FEATURE_MODERN_LIBS

    *(void **)(cinfo + 0x18) = Z_MallocJpeg;
    *(void **)(cinfo + 0x1c) = Z_FreeJpeg;
#endif

    jpeg_CreateDecompress(cinfo, 0x3e, JPEG_D_STRUCT_SIZE);
#if COD2_FEATURE_MODERN_LIBS
    jpeg_mem_src(cinfo, fbuffer, (unsigned long)filesize);
#else
    jpeg_memory_src(cinfo, fbuffer, filesize);
#endif
    jpeg_read_header(cinfo, 1);
    jpeg_start_decompress(cinfo);

    output_width = *(int *)(cinfo + JPEG_D_OUTPUT_WIDTH);
    output_height = *(int *)(cinfo + JPEG_D_OUTPUT_HEIGHT);

    if (output_width <= 0 || output_height <= 0 ||
        (long long)output_width * (long long)output_height * 4LL > (long long)0x7fffffff) {
        ((refimport_t *)sys)->Printf(2, "WARNING: jpeg image '%s' has invalid dimensions\n", filepath);
        jpeg_destroy_decompress(cinfo);
        ((refimport_t *)sys)->FS_FreeFile(fbuffer);
        return;
    }

    vidConfig_t *vidCfg = &vidConfig;
    maxSize = vidCfg->maxTextureSize;
    if (output_width > maxSize || output_height > maxSize) {
        ((refimport_t *)sys)->Printf(2, "WARNING: image '%s' is larger than %i on at least one side\n", filepath, maxSize);
        jpeg_destroy_decompress(cinfo);
        ((refimport_t *)sys)->FS_FreeFile(fbuffer);
        return;
    }

    num_components = *(int *)(cinfo + JPEG_D_OUT_COLOR_COMPS);
    if (num_components != 3) {
        refimport_t *ri2 = &ri;
        ri2->Printf(2, "WARNING: jpeg image '%s' is not RGB\n", filepath);
        jpeg_destroy_decompress(cinfo);
        ri2->FS_FreeFile(fbuffer);
        return;
    }

    *width = output_width;
    *height = output_height;
    *imageFormat = (D3DFORMAT)0x15;

    sys = (byte *)&ri;
    buf = ((byte * (*)(int)) * (void **)(sys + 0x10))(output_width * output_height * 4);
    *pic = buf;

    row_stride = output_width * 4;
    *file = fbuffer;

    while (output_height > *(int *)(cinfo + JPEG_D_OUTPUT_SCANLINE)) {
        row_ptr = buf;
        jpeg_read_scanlines(cinfo, &row_ptr, 1);

        output_width = *(int *)(cinfo + JPEG_D_OUTPUT_WIDTH);
        for (x = output_width - 1; x >= 0; x--) {
            int srcOffset = x * num_components;
            buf[x * 4 + 3] = 0xff;
            buf[x * 4 + 2] = buf[srcOffset + 2];
            buf[x * 4 + 1] = buf[srcOffset + 1];
            buf[x * 4 + 0] = buf[srcOffset + 0];
        }

        buf += row_stride;
    }

    jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);
}
