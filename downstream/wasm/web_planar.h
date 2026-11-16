#ifndef WEB_PLANAR_H
#define WEB_PLANAR_H

void WebPlanar_BeginFrame(void);
void WebPlanar_EndFrame(void);
int WebPlanar_BeginDraw(int eligible);
int WebPlanar_IsDrawing(void);
void WebPlanar_EndDraw(void);
void WebPlanar_MirrorView(float *view);
void WebPlanar_Material(const float *view);
void WebPlanar_Candidate(const Material *material, const float *positions,
                         int stride, int count, const unsigned short *indices, int indexCount,
                         int indexBase, const float *world, const float *view);

#endif
