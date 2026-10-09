#ifndef IMPORTS_AGL_H
#define IMPORTS_AGL_H

int aglChoosePixelFormat();
int aglCreateContext();
int aglDescribePixelFormat();
int aglDestroyContext();
int aglDestroyPixelFormat();
int aglGetDrawable();
int aglSetCurrentContext();
int aglSetDrawable();
int aglSetFullScreen();
int aglSetInteger();
void aglSwapBuffers(void *context);

#endif
