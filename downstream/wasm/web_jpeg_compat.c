#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

typedef struct {
  struct jpeg_source_mgr pub;
  boolean empty_input;
} cod2_memory_source_mgr;

typedef cod2_memory_source_mgr *cod2_memory_source_ptr;

static const JOCTET cod2_jpeg_eoi[2] = { 0xFF, JPEG_EOI };

METHODDEF(void)
cod2_memory_init_source(j_decompress_ptr cinfo)
{
  (void)cinfo;
}

METHODDEF(boolean)
cod2_memory_fill_input_buffer(j_decompress_ptr cinfo)
{
  cod2_memory_source_ptr src = (cod2_memory_source_ptr)cinfo->src;

  if (src->empty_input)
    ERREXIT(cinfo, JERR_INPUT_EMPTY);

  WARNMS(cinfo, JWRN_JPEG_EOF);
  src->pub.next_input_byte = cod2_jpeg_eoi;
  src->pub.bytes_in_buffer = sizeof(cod2_jpeg_eoi);
  return TRUE;
}

METHODDEF(void)
cod2_memory_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *src = cinfo->src;

  if (num_bytes <= 0)
    return;

  if ((unsigned long)num_bytes <= src->bytes_in_buffer) {
    src->next_input_byte += (size_t)num_bytes;
    src->bytes_in_buffer -= (size_t)num_bytes;
    return;
  }

  num_bytes -= (long)src->bytes_in_buffer;
  src->next_input_byte += src->bytes_in_buffer;
  src->bytes_in_buffer = 0;
  if (num_bytes > 0) {
    (void)cod2_memory_fill_input_buffer(cinfo);
    if ((unsigned long)num_bytes > src->bytes_in_buffer)
      num_bytes = (long)src->bytes_in_buffer;
    src->next_input_byte += (size_t)num_bytes;
    src->bytes_in_buffer -= (size_t)num_bytes;
  }
}

METHODDEF(void)
cod2_memory_term_source(j_decompress_ptr cinfo)
{
  (void)cinfo;
}

void jpeg_memory_src(void *opaque_cinfo, unsigned char *data, int size)
{
  j_decompress_ptr cinfo = (j_decompress_ptr)opaque_cinfo;
  cod2_memory_source_ptr src;

  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)(
      (j_common_ptr)cinfo, JPOOL_PERMANENT, SIZEOF(cod2_memory_source_mgr));
  }

  src = (cod2_memory_source_ptr)cinfo->src;
  src->pub.init_source = cod2_memory_init_source;
  src->pub.fill_input_buffer = cod2_memory_fill_input_buffer;
  src->pub.skip_input_data = cod2_memory_skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = cod2_memory_term_source;
  src->pub.next_input_byte = data;
  src->pub.bytes_in_buffer = size > 0 ? (size_t)size : 0;
  src->empty_input = size <= 0;
}
