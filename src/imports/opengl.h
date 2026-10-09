#ifndef IMPORTS_OPENGL_H
#define IMPORTS_OPENGL_H

#if defined(__EMSCRIPTEN__)
#include <GL/gl.h>
#include <GL/glext.h>
#include "downstream/wasm/web_gl_compat.h"
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
void glLightModelfv(unsigned int pname, const float *params);
int glLightModeli();
void glLightf(unsigned int light, unsigned int pname, float param);
void glLightfv(unsigned int light, unsigned int pname, const float *params);
void glLoadIdentity(void);
int glLoadMatrixf();
void glMaterialf(unsigned int face, unsigned int pname, float param);
void glMaterialfv(unsigned int face, unsigned int pname, const float *params);
void glMatrixMode(unsigned int mode);
void glNormalPointer(unsigned int type, int stride, const void *pointer);
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
void glTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels);
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
