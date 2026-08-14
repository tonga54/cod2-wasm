#ifndef IMPORTS_OPENGL_H
#define IMPORTS_OPENGL_H

#if defined(__EMSCRIPTEN__)

#if defined(COD2_DOWNSTREAM_WASM)
#include "downstream/wasm/web_gl_compat.h"
#else
#include "web/webgl2_compat.h"
#endif

void glBegin(unsigned int mode);
void glBindTexture(unsigned int target, unsigned int texture);
int glBindVertexArrayAPPLE(unsigned int array);
int glBlendEquationEXT(unsigned int mode);
int glBlendFuncSeparateEXT(unsigned int srcRGB, unsigned int dstRGB, unsigned int srcAlpha, unsigned int dstAlpha);
int glClipPlane();
int glColorMaterial();
int glCombinerParameterfvNV();
int glCombinerStageParameterfvNV();
int glCompressedTexImage2DARB(unsigned int target, int level, unsigned int internalformat, int width, int height, int border, int imageSize, const void *data);
int glCompressedTexImage3DARB(unsigned int target, int level, unsigned int internalformat, int width, int height, int depth, int border, int imageSize, const void *data);
void glCompressedTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, int imageSize, const void *data);
void glCompressedTexSubImage3D(unsigned int target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, unsigned int format, int imageSize, const void *data);
void glCopyTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int x, int y, int width, int height);
int glDeleteFencesAPPLE();
int glDeleteProgramsARB();
void glDeleteTextures(int n, const unsigned int *textures);
int glDeleteVertexArraysAPPLE(int n, const unsigned int *arrays);
int glDisableVertexAttribArrayARB(unsigned int index);
int glDrawBuffer();
int glDrawRangeElements();
int glEnableVertexAttribArrayARB(unsigned int index);
void glEnd(void);
void glFinish(void);
int glFinishFenceAPPLE();
int glFlushVertexArrayRangeAPPLE();
void glFogf(unsigned int pname, float param);
int glFogi();
int glFrontFace();
int glGenFencesAPPLE();
int glGenProgramsARB();
void glGenTextures(int n, unsigned int *textures);
int glGenVertexArraysAPPLE(int n, unsigned int *arrays);
int glGetFloatv();
int glGetProgramivARB();
const unsigned char *glGetString();
int glHint();
int glLightModelfv();
int glLightModeli();
void glLightf(unsigned int light, unsigned int pname, float param);
int glLightfv();
void glMaterialf(unsigned int face, unsigned int pname, float param);
int glMaterialfv();
int glNormalPointer();
void glPixelStorei(unsigned int pname, int param);
void glPointParameterfARB(unsigned int pname, float param);
int glPointParameterfvARB();
void glPointSize(float size);
int glPolygonMode();
void glPolygonOffset(float factor, float units);
int glPopAttrib();
int glPopClientAttrib();
void glPopMatrix(void);
int glProgramStringARB();
int glPushAttrib();
int glPushClientAttrib();
void glPushMatrix(void);
void glReadBuffer(unsigned int src);
void glReadPixels(int x, int y, int width, int height, unsigned int format, unsigned int type, void *pixels);
void glScissor(int x, int y, int width, int height);
int glSetFenceAPPLE();
int glShadeModel();
int glStencilFunc();
int glStencilMask();
int glStencilOp();
int glTestFenceAPPLE();
void glTexCoord2f(float s, float t);
void glTexEnvf(unsigned int target, unsigned int pname, float param);
int glTexGenfv();
int glTexGeni();
void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels);
void glTexImage3D(unsigned int target, int level, int internalformat, int width, int height, int depth, int border, unsigned int format, unsigned int type, const void *pixels);
void glTexParameterf(unsigned int target, unsigned int pname, float param);
int glTexParameterfv();
void glTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels);
void glTexSubImage3D(unsigned int target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, unsigned int format, unsigned int type, const void *pixels);
void glVertex2f(float x, float y);
void glVertex3f(float x, float y, float z);
int glVertexArrayParameteriAPPLE();
int glVertexArrayRangeAPPLE();
int glVertexAttribPointerARB(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void *pointer);
int gluCheckExtension();
void gluOrtho2D(double l, double r, double b, double t);

#define glDisable webgl2_glDisable
#define glDrawArrays webgl2_glDrawArrays
#define glDrawElements webgl2_glDrawElements
#define glDrawRangeElements webgl2_glDrawRangeElements
#define glEnable webgl2_glEnable

#else

int glActiveTextureARB();
void glAlphaFunc(unsigned int func, float ref);
void glBegin(unsigned int mode);
int glBindProgramARB();
void glBindTexture(unsigned int target, unsigned int texture);
int glBindVertexArrayAPPLE();
int glBlendEquationEXT();
void glBlendFunc(unsigned int sfactor, unsigned int dfactor);
int glBlendFuncSeparateEXT();
void glClear(unsigned int mask);
void glClearColor(float r, float g, float b, float a);
void glClearDepth(double depth);
int glClearStencil();
int glClientActiveTextureARB();
int glClipPlane();
void glColor4f(float r, float g, float b, float a);
int glColorMask();
int glColorMaterial();
int glColorPointer();
int glCombinerParameterfvNV();
int glCombinerStageParameterfvNV();
int glCompressedTexImage2DARB();
int glCompressedTexImage3DARB();
int glCompressedTexSubImage2D();
int glCompressedTexSubImage3D();
int glCopyTexSubImage2D();
int glDeleteFencesAPPLE();
int glDeleteProgramsARB();
void glDeleteTextures(int n, const unsigned int *textures);
int glDeleteVertexArraysAPPLE();
int glDepthFunc();
void glDepthMask(unsigned char flag);
void glDepthRange(double n, double f);
void glDisable(unsigned int cap);
int glDisableClientState();
int glDisableVertexAttribArrayARB();
int glDrawBuffer();
int glDrawRangeElements();
void glEnable(unsigned int cap);
int glEnableClientState();
int glEnableVertexAttribArrayARB();
void glEnd(void);
int glFinish();
int glFinishFenceAPPLE();
int glFlushVertexArrayRangeAPPLE();
void glFogf(unsigned int pname, float param);
int glFogfv();
int glFogi();
int glFrontFace();
int glGenFencesAPPLE();
int glGenProgramsARB();
void glGenTextures(int n, unsigned int *textures);
int glGenVertexArraysAPPLE();
unsigned int glGetError(void);
int glGetFloatv();
void glGetIntegerv(unsigned int pname, int *params);
int glGetProgramivARB();
const unsigned char *glGetString();
int glHint();
int glLightModelfv();
int glLightModeli();
void glLightf(unsigned int light, unsigned int pname, float param);
int glLightfv();
void glLoadIdentity(void);
int glLoadMatrixf();
void glMaterialf(unsigned int face, unsigned int pname, float param);
int glMaterialfv();
void glMatrixMode(unsigned int mode);
int glNormalPointer();
int glPixelStorei();
void glPointParameterfARB(unsigned int pname, float param);
int glPointParameterfvARB();
void glPointSize(float size);
int glPolygonMode();
void glPolygonOffset(float factor, float units);
int glPopAttrib();
int glPopClientAttrib();
void glPopMatrix(void);
int glProgramEnvParameter4fvARB();
int glProgramStringARB();
int glPushAttrib();
int glPushClientAttrib();
void glPushMatrix(void);
int glReadBuffer();
int glReadPixels();
void glScalef(float x, float y, float z);
void glScissor(int x, int y, int width, int height);
int glSetFenceAPPLE();
int glShadeModel();
int glStencilFunc();
int glStencilMask();
int glStencilOp();
int glTestFenceAPPLE();
void glTexCoord2f(float s, float t);
int glTexCoordPointer();
void glTexEnvf(unsigned int target, unsigned int pname, float param);
int glTexEnvfv();
int glTexEnvi();
int glTexGenfv();
int glTexGeni();
void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels);
int glTexImage3D();
void glTexParameterf(unsigned int target, unsigned int pname, float param);
int glTexParameterfv();
void glTexParameteri(unsigned int target, unsigned int pname, int param);
int glTexSubImage2D();
int glTexSubImage3D();
void glVertex2f(float x, float y);
void glVertex3f(float x, float y, float z);
int glVertexArrayParameteriAPPLE();
int glVertexArrayRangeAPPLE();
int glVertexAttribPointerARB();
int glVertexPointer();
void glViewport(int x, int y, int width, int height);
int gluCheckExtension();
void gluOrtho2D(double l, double r, double b, double t);

#endif

#endif
