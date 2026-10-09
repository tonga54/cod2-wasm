#include <GLES3/gl3.h>
#include <stdlib.h>

/* Extension spellings map to actual WebGL operations, not desktop loaders. */
extern void glClientActiveTexture(unsigned int texture);
void glActiveTextureARB(unsigned int texture) { glActiveTexture(texture); }
void glClientActiveTextureARB(unsigned int texture) { glClientActiveTexture(texture); }
void glCompressedTexImage2DARB(unsigned int target, int level, unsigned int format,
                             int width, int height, int border, int size, const void *data) {
  glCompressedTexImage2D(target, level, format, width, height, border, size, data);
}
void glCompressedTexImage3DARB(unsigned int target, int level, unsigned int format,
                             int width, int height, int depth, int border,
                             int size, const void *data) {
  glCompressedTexImage3D(target, level, format, width, height, depth, border, size, data);
}

/* WebGL2 exposes the plural API even for the single default back buffer. */
void emscripten_glDrawBuffer(unsigned int buffer) {
  GLenum target = buffer;
  glDrawBuffers(1, &target);
}
void glDrawBuffer(unsigned int buffer) { emscripten_glDrawBuffer(buffer); }

/* Keep GPU queries asynchronous. Browser startup must select r_gpuSync off:
 * a main-thread spin loop prevents the browser from advancing GPU queries. */
#define WEB_FENCE_COUNT 64
static struct { int allocated; GLsync sync; } webFences[WEB_FENCE_COUNT];
void glGenFencesAPPLE(int count, unsigned int *ids) {
  for (int i = 0; i < count; ++i) {
    int slot;
    for (slot = 1; slot < WEB_FENCE_COUNT && webFences[slot].allocated; ++slot) {}
    if (slot == WEB_FENCE_COUNT) abort();
    webFences[slot].allocated = 1;
    ids[i] = slot;
  }
}
void glDeleteFencesAPPLE(int count, const unsigned int *ids) {
  for (int i = 0; i < count; ++i) {
    unsigned int id = ids[i];
    if (!id || id >= WEB_FENCE_COUNT) continue;
    if (webFences[id].sync) glDeleteSync(webFences[id].sync);
    webFences[id].sync = 0;
    webFences[id].allocated = 0;
  }
}
void glSetFenceAPPLE(unsigned int id) {
  if (!id || id >= WEB_FENCE_COUNT || !webFences[id].allocated) abort();
  if (webFences[id].sync) glDeleteSync(webFences[id].sync);
  webFences[id].sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
}
unsigned char glTestFenceAPPLE(unsigned int id) {
  if (!id || id >= WEB_FENCE_COUNT || !webFences[id].allocated) abort();
  if (!webFences[id].sync) return 0;
  GLenum result = glClientWaitSync(webFences[id].sync, 0, 0);
  if (result == GL_WAIT_FAILED) abort();
  return result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
}

void webgl2_glDisable(unsigned int capability) { glDisable(capability); }
void webgl2_glEnable(unsigned int capability) { glEnable(capability); }
void webgl2_glDrawArrays(unsigned int mode, int first, int count) {
  glDrawArrays(mode, first, count);
}
void webgl2_glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices) {
  glDrawElements(mode, count, type, indices);
}
void webgl2_glDrawRangeElements(unsigned int mode, unsigned int start, unsigned int end,
                                int count, unsigned int type, const void *indices) {
  glDrawRangeElements(mode, start, end, count, type, indices);
}
