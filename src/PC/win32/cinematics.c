#include "common_types.h"
#include "imports.h"
extern clientStatic_t cls;

extern void Com_DPrintf(const char *fmt, ...);
extern void SND_StopSounds(int);
extern void SND_EndRawSamples(void);
extern void Sys_EndStreamedFile(fileHandle_t);
extern void Sys_BeginStreamedFile(fileHandle_t, int);
extern int Sys_StreamedRead(void *, int, int, fileHandle_t);
extern void FS_FCloseFile(fileHandle_t);
extern int FS_FOpenFileRead(const char *, fileHandle_t *, int);
extern char *va(const char *fmt, ...);
extern void Cbuf_ExecuteText(int, const char *);
extern void Dvar_SetString(void *, const char *);
extern qboolean UI_SetActiveMenu(int);
extern int CL_ScaledMilliseconds(void);
extern char *Cmd_Argv(int);
extern void CL_StopLogo(void);
extern void SND_FadeAllSounds(float, int);
extern void CL_LookupColor(int, float *);
extern float floorf(float);
extern float ceilf(float);

static Boolean sAspyrIntroPlayed;
extern long int ROQ_YY_tab[256];
extern long int ROQ_UB_tab[256];
extern long int ROQ_UG_tab[256];
extern long int ROQ_VG_tab[256];
extern long int ROQ_VR_tab[256];
extern short unsigned int vq2[16384];
extern short unsigned int vq4[65536];
extern short unsigned int vq8[262144];
extern struct cinematics_t cin;
extern cin_cache cinTable[16];
__attribute__((used)) int CL_handle = -1;
__attribute__((used, section(".data.currentHandle"), aligned(4))) int currentHandle = -1;
__attribute__((used, section(".data.currentHandle"), aligned(4))) static int currentHandle_pad[6];
;

static void recurseQuad(long int startX, long int startY, long int quadSize, long int xOff, long int yOff);
static void RoQPrepMcomp(long int xoff, long int yoff);
static void RoQShutdown(void);
e_status ROQ_StopCinematicFromHandle(int handle);
void ROQ_SetExtentsFromHandle(int handle, int x, int y, int w, int h);
void ROQ_UploadCinematicFromHandle(int handle);
static void blitVQQuad32fs(byte **status, unsigned char *data);
int ROQ_PlayCinematic(const char *arg, int x, int y, int w, int h, int systemBits);
void ROQ_DrawCinematicFromHandle(int handle);
void ROQ_DrawCinematic(void);
static void RoQReset(void);
void ROQ_CloseAllVideos(void);
void ROQ_StopCinematic(void);
e_status ROQ_RunCinematicFromHandle(int handle);
void ROQ_RunCinematic(void);
void ROQ_PlayCinematic_f(void);

static qboolean ROQ_SoundEnabled(void)
{
    byte *sndGlob = (byte *)(snd_local_t *)imp_g_snd;
    return sndGlob != NULL && *sndGlob != 0;
}

static inline void RoQ_VQ2To4RGBA(unsigned int **a, unsigned int **b, unsigned int **c, unsigned int **d)
{
    unsigned int *ap = *a;
    unsigned int *bp = *b;
    unsigned int *cp = *c;
    unsigned int *dp = *d;

    *cp++ = ap[0];
    *dp++ = ap[0];
    *dp++ = ap[0];
    *cp++ = ap[1];
    *dp++ = ap[1];
    *dp++ = ap[1];
    *cp++ = bp[0];
    *dp++ = bp[0];
    *dp++ = bp[0];
    *cp++ = bp[1];
    *dp++ = bp[1];
    *dp++ = bp[1];
    *dp++ = ap[0];
    *dp++ = ap[0];
    *dp++ = ap[1];
    *dp++ = ap[1];
    *dp++ = bp[0];
    *dp++ = bp[0];
    *dp++ = bp[1];
    *dp++ = bp[1];

    *a = ap + 2;
    *b = bp + 2;
    *c = cp;
    *d = dp;
}

static void recurseQuad(long int startX, long int startY, long int quadSize, long int xOff, long int yOff)
{
    int h;
    long int offset;
    long int bigx;
    long int bigy;
    long int scrY;
    long int scrX;
    long int onQuad;

    h = currentHandle;
    offset = cinTable[h].screenDelta;
    bigx = cinTable[h].xsize;
    bigy = cinTable[h].ysize;

    if (bigx > cinTable[h].CIN_WIDTH) {
        bigx = cinTable[h].CIN_WIDTH;
    }
    if (bigy > cinTable[h].CIN_HEIGHT) {
        bigy = cinTable[h].CIN_HEIGHT;
    }

    if (startX >= 0 && startY >= 0 &&
        startX + quadSize <= bigx &&
        startY + quadSize <= bigy &&
        quadSize <= 8) {
        scrY = (startY + ((cinTable[h].CIN_HEIGHT - bigy) >> 1) + yOff) * cinTable[h].samplesPerLine;
        scrX = (startX + xOff) * cinTable[h].samplesPerPixel;
        onQuad = cinTable[h].onQuad;
        cin.qStatus[0][onQuad] = (byte *)((char *)&cin + scrY + scrX);
        cin.qStatus[1][onQuad] = (byte *)((char *)&cin + scrY + scrX + offset);
        cinTable[h].onQuad = onQuad + 1;
    }

    if (quadSize != 4) {
        quadSize >>= 1;
        recurseQuad(startX, startY, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY, quadSize, xOff, yOff);
        recurseQuad(startX, startY + quadSize, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY + quadSize, quadSize, xOff, yOff);
    }
}

static void RoQPrepMcomp(long int xoff, long int yoff)
{
    long int i;
    long int j;
    long int x;
    long int y;
    long int temp;
    long int temp2;

    i = cinTable[currentHandle].samplesPerLine;
    j = cinTable[currentHandle].samplesPerPixel;
    if (cinTable[currentHandle].xsize == cinTable[currentHandle].ysize * 4 && !cinTable[currentHandle].half) {
        j += j;
        i += i;
    }

    for (y = 0; y < 16; y++) {
        temp2 = (y + yoff - 8) * i;
        for (x = 0; x < 16; x++) {
            temp = (x + xoff - 8) * j;
            cin.mcomp[x * 16 + y] = cinTable[currentHandle].normalBuffer0 - (temp2 + temp);
        }
    }
}

static void RoQShutdown(void)
{
    int h;
    int *clcState;
    char *nextMapStr;
    void (*rendererCallback)(void);

    h = currentHandle;
    if (cinTable[h].buf && cinTable[h].status != FMV_IDLE) {
        Com_DPrintf("finished cinematic\n");
        cinTable[currentHandle].status = FMV_IDLE;

        if (cinTable[currentHandle].sound) {
            SND_EndRawSamples();
        }

        if (cinTable[currentHandle].iFile) {
            Sys_EndStreamedFile(cinTable[currentHandle].iFile);
            FS_FCloseFile(cinTable[currentHandle].iFile);
            cinTable[currentHandle].iFile = 0;
        }

        if (cinTable[currentHandle].alterGameState) {
            clcState = (int *)(void *)imp_clc;
            *clcState = cinTable[currentHandle].previousGameState;
            CL_handle = -1;
            if (*clcState == 0) {
                nextMapStr = (*(dvar_t **)imp_nextmap)->current.string;
                if (*nextMapStr != '\0') {
                    if (!sAspyrIntroPlayed) {
                        if (strstr(nextMapStr, "IW_logo")) {
                            Cbuf_ExecuteText(2, va("%s\n", "cinematic aspyr_intro"));
                            sAspyrIntroPlayed = 1;
                        } else {
                            Cbuf_ExecuteText(2, va("%s\n", nextMapStr));
                            Dvar_SetString(*(dvar_t **)imp_nextmap, "");
                        }
                    } else {
                        Cbuf_ExecuteText(2, va("%s\n", nextMapStr));
                        Dvar_SetString(*(dvar_t **)imp_nextmap, "");
                    }
                    UI_SetActiveMenu(1);
                } else {
                    UI_SetActiveMenu(0);
                }
            } else {
                UI_SetActiveMenu(0);
            }
        }

        rendererCallback = ((refexport_t *)(void *)imp_re)->SyncRenderThread;
        if (rendererCallback) {
            rendererCallback();
        }

        memset(&cinTable[currentHandle], 0, sizeof(cin_cache));
        currentHandle = -1;
    }
}

e_status ROQ_StopCinematicFromHandle(int handle)
{
    if ((unsigned int)handle > 0xf || cinTable[handle].status == FMV_EOF) {
        return FMV_EOF;
    }
    currentHandle = handle;
    Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);
    if (!cinTable[currentHandle].buf) {
        return FMV_EOF;
    }
    if (cinTable[currentHandle].alterGameState) {
        if (*(int *)(void *)imp_clc != 1) {
            return cinTable[currentHandle].status;
        }
    }
    cinTable[currentHandle].status = FMV_EOF;
    RoQShutdown();
    return FMV_EOF;
}

void ROQ_SetExtentsFromHandle(int handle, int x, int y, int w, int h)
{
    if ((unsigned int)handle <= 0xf && cinTable[handle].status != FMV_EOF) {
        cinTable[handle].xpos = x;
        cinTable[handle].ypos = y;
        cinTable[handle].width = w;
        cinTable[handle].height = h;
        cinTable[handle].dirty = 1;
    }
}

void ROQ_UploadCinematicFromHandle(int handle)
{
    if ((unsigned int)handle <= 0xf && cinTable[handle].buf) {
        if (cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty) {
            if (cinTable[handle].playonwalls == 0) {
                cinTable[handle].playonwalls = -1;
            } else if (cinTable[handle].playonwalls == -1) {
                cinTable[handle].playonwalls = -2;
            } else {
                cinTable[handle].dirty = 0;
            }
        }
        if (!(*(dvar_t **)imp_cl_inGameVideo)->current.enabled) {
            if (cinTable[handle].playonwalls == 1) {
                cinTable[handle].playonwalls = 0;
            }
        }
    }
}

static void move8_32(byte *src, byte *dst, int spl)
{
    double *dsrc;
    double *ddst;
    int dspl;

    dsrc = (double *)src;
    ddst = (double *)dst;
    dspl = spl >> 3;

    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
}

static void move4_32(byte *src, byte *dst, int spl)
{
    double *dsrc;
    double *ddst;
    int dspl;

    dsrc = (double *)src;
    ddst = (double *)dst;
    dspl = spl >> 3;

    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += dspl;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
}

static void blit8_32(byte *src, byte *dst, int spl)
{
    double *dsrc;
    double *ddst;
    int dspl;

    dsrc = (double *)src;
    ddst = (double *)dst;
    dspl = spl >> 3;

    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
    dsrc += 4;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    ddst[2] = dsrc[2];
    ddst[3] = dsrc[3];
}

static void blit4_32(byte *src, byte *dst, int spl)
{
    double *dsrc;
    double *ddst;
    int dspl;

    dsrc = (double *)src;
    ddst = (double *)dst;
    dspl = spl >> 3;

    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += 2;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += 2;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
    dsrc += 2;
    ddst += dspl;
    ddst[0] = dsrc[0];
    ddst[1] = dsrc[1];
}

static void blit2_32(byte *src, byte *dst, int spl)
{
    double *dsrc;
    double *ddst;
    int dspl;

    dsrc = (double *)src;
    ddst = (double *)dst;
    dspl = spl >> 3;

    ddst[0] = dsrc[0];
    ddst[dspl] = dsrc[1];
}

static void blitVQQuad32fs(byte **status, unsigned char *data)
{
    unsigned short newd;
    unsigned short celdata;
    unsigned short code;
    unsigned int index;
    unsigned int i;
    int spl;

    newd = 0;
    celdata = 0;
    index = 0;
    spl = cinTable[currentHandle].samplesPerLine;

    do {
        if (!newd) {
            newd = 7;
            celdata = data[0] + data[1] * 256;
            data += 2;
        } else {
            newd--;
        }

        code = (unsigned short)(celdata & 0xc000);
        celdata <<= 2;

        switch (code) {
        case 0x8000:
            blit8_32((byte *)&vq8[(*data) * 128], status[index], spl);
            data++;
            index += 5;
            break;

        case 0xc000:
            index++;
            for (i = 0; i < 4; i++) {
                if (!newd) {
                    newd = 7;
                    celdata = data[0] + data[1] * 256;
                    data += 2;
                } else {
                    newd--;
                }

                code = (unsigned short)(celdata & 0xc000);
                celdata <<= 2;

                switch (code) {
                case 0x8000:
                    blit4_32((byte *)&vq4[(*data) * 32], status[index], spl);
                    data++;
                    break;

                case 0xc000:
                    blit2_32((byte *)&vq2[(*data) * 8], status[index], spl);
                    data++;
                    blit2_32((byte *)&vq2[(*data) * 8], status[index] + 8, spl);
                    data++;
                    blit2_32((byte *)&vq2[(*data) * 8], status[index] + spl * 2, spl);
                    data++;
                    blit2_32((byte *)&vq2[(*data) * 8], status[index] + spl * 2 + 8, spl);
                    data++;
                    break;

                case 0x4000:
                    move4_32(status[index] + cin.mcomp[*data], status[index], spl);
                    data++;
                    break;
                }
                index++;
            }
            break;

        case 0x4000:
            move8_32(status[index] + cin.mcomp[*data], status[index], spl);
            data++;
            index += 5;
            break;

        case 0x0000:
            index += 5;
            break;
        }
    } while (status[index] != NULL);
}

extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int I_stricmp(const char *s1, const char *s2);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Memset(void *dest, int val, int count);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern void SND_RawSamples(int samples, int rate, int width, int channels, const byte *data);
extern void Con_Close(void);

static int CIN_findHandle(void)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (cinTable[i].fileName[0] == '\0') {
            return i;
        }
    }
    Com_Error(1, "CIN_findHandle: none free");
    return 0;
}

int ROQ_PlayCinematic(const char *arg, int x, int y, int w, int h, int systemBits)
{
    char name[256];
    unsigned char looping;
    int i;
    int handle;
    unsigned short id;
    int fps;

    Com_sprintf(name, 256, "video/%s.roq", arg);
    looping = (unsigned char)systemBits & 1;

    if (!looping) {
        for (i = 0; i < 16; i++) {
            if (I_stricmp(cinTable[i].fileName, name) == 0) {
                return i;
            }
        }
    }

    Com_DPrintf("SCR_PlayCinematic( %s )\n", arg);
    Com_Memset(&cin, 0, sizeof(cin));
    cin.currentHandle = -1;
    handle = CIN_findHandle();
    currentHandle = handle;

    strcpy(cinTable[handle].fileName, name);
    cinTable[handle].ROQSize = 0;
    cinTable[handle].ROQSize = FS_FOpenFileRead(cinTable[handle].fileName, &cinTable[handle].iFile, 1);

    if (cinTable[handle].ROQSize <= 0) {
        Com_DPrintf("play(%s), ROQSize<=0\n", arg);
        cinTable[currentHandle].fileName[0] = '\0';
        return -1;
    }

    if ((unsigned int)handle <= 0xf && cinTable[handle].status != FMV_EOF) {
        cinTable[handle].xpos = x;
        cinTable[handle].ypos = y;
        cinTable[handle].width = w;
        cinTable[handle].height = h;
        cinTable[handle].dirty = 1;
    }

    {
        int loop = (systemBits >> 1) & 1;
        if ((unsigned int)handle <= 0xf && cinTable[handle].status != FMV_EOF) {
            cinTable[handle].looping = loop;
        }
    }

    cinTable[handle].holdAtEnd = (systemBits >> 2) & 1;
    cinTable[handle].alterGameState = looping;
    cinTable[handle].playonwalls = 1;
    cinTable[handle].silent = (systemBits >> 3) & 1;
    cinTable[handle].material = (systemBits >> 4) & 1;
    cinTable[handle].letterBox = (systemBits >> 5) & 1;
    cinTable[handle].sound = 0;

    if (cinTable[handle].alterGameState) {
        if (cls.uiStarted) {
            UI_SetActiveMenu(0);
        }
    } else {
        cinTable[handle].playonwalls = (*(dvar_t **)imp_cl_inGameVideo)->current.enabled != 0;
    }

    if (handle >= 0) {
        cinTable[handle].VQ0 = blitVQQuad32fs;
        cinTable[handle].VQ1 = blitVQQuad32fs;
        cinTable[handle].VQNormal = blitVQQuad32fs;
        cinTable[handle].VQBuffer = blitVQQuad32fs;
        cinTable[handle].samplesPerPixel = 4;

        for (i = 0; i < 256; i++) {
            float f = (float)(i * 2 - 255);
            ROQ_UB_tab[i] = (long int)(f * 57.204f + 32.0f);
            ROQ_VR_tab[i] = (long int)(f * 45.364f + 32.0f);
            ROQ_UG_tab[i] = (long int)(f * -11.5125f);
            ROQ_VG_tab[i] = (long int)(f * -23.3525f + 32.0f);
            ROQ_YY_tab[i] = (long int)((i << 6) | (i >> 2));
        }

        {
            int d;
            short *tab = cin.sqrTable;
            for (d = 0; d < 128; d++) {
                tab[d] = (short)(d * d);
                tab[d + 128] = (short)(-(d * d));
            }
        }
    }

    FS_Read(cin.file, 16, cinTable[handle].iFile);

    id = cin.file[0] | (cin.file[1] << 8);
    if (id != 0x1084) {
        Com_DPrintf("trFMV::play(), invalid RoQ ID\n");
        RoQShutdown();
        return -1;
    }

    {
        unsigned int t;
        t = CL_ScaledMilliseconds();
        cinTable[currentHandle].startTime = t;
        cinTable[currentHandle].lastTime = t;
        cinTable[currentHandle].lastFrameTime = t;
        cinTable[currentHandle].RoQPlayed = 24;

        fps = cin.file[6] | (cin.file[7] << 8);
        if (fps == 0)
            fps = 30;
        cinTable[currentHandle].roqFPS = fps;
        cinTable[currentHandle].numQuads = -1;
        cinTable[currentHandle].roq_id = cin.file[8] | (cin.file[9] << 8);
        cinTable[currentHandle].RoQFrameSize = cin.file[10] | (cin.file[11] << 8) | (cin.file[12] << 16);
        cinTable[currentHandle].roq_flags = cin.file[14] | (cin.file[15] << 8);
        cinTable[currentHandle].roqF0 = (signed char)cin.file[15];
        cinTable[currentHandle].roqF1 = (signed char)cin.file[14];
    }

    Sys_BeginStreamedFile(cinTable[currentHandle].iFile, 0x10000);
    cinTable[currentHandle].letterBox = 1;

    Com_DPrintf("trFMV::play(), playing %s\n", arg);

    if (cinTable[currentHandle].alterGameState) {
        int *clcState = (int *)(void *)imp_clc;
        cinTable[currentHandle].previousGameState = (connstate_t)*clcState;
        *clcState = 1;
    }

    Con_Close();
    i = currentHandle;
    return i;
}

void ROQ_DrawCinematicFromHandle(int handle)
{
    byte *buf;
    float aspect;
    float clearColor[4];
    int x, y, w, h;
    clientStatic_t *cls;
    refexport_t *re;
    float screenAspect;
    int screenW, screenH;
    float barSize;
    int barSizeCeil;
    float xf, wf;
    int material;

    if ((unsigned int)handle > 0xf || cinTable[handle].status == FMV_EOF) {
        goto done;
    }
    buf = cinTable[handle].buf;
    if (!buf) {
        goto done;
    }

    aspect = (float)cinTable[handle].width / (float)cinTable[handle].height;
    CL_LookupColor(0x30, clearColor);

    cls = (clientStatic_t *)imp_cls;
    screenAspect = cls->vidConfig.aspectRatioWindow;

    if (aspect == screenAspect) {

        w = cls->vidConfig.width;
        h = cls->vidConfig.height;
        x = 0;
        y = 0;
    } else if (aspect > screenAspect) {

        h = cls->vidConfig.height;
        barSize = ((float)h - screenAspect / aspect * (float)h) * 0.5f;
        y = (int)floorf(barSize);
        barSizeCeil = (int)ceilf(barSize);
        h = h - barSizeCeil * 2;
        w = cls->vidConfig.width;
        x = 0;
    } else {

        w = cls->vidConfig.width;
        barSize = ((float)w - aspect / screenAspect * (float)w) * 0.5f;
        x = (int)floorf(barSize);
        barSizeCeil = (int)ceilf(barSize);
        w = w - barSizeCeil * 2;
        h = cls->vidConfig.height;
        y = 0;
    }

    re = (refexport_t *)(void *)imp_re;
    ((void (*)(int, const vec_t *, float, int))re->ClearScreen)(1, clearColor, 0.0f, 0);

    ((void (*)(int, int, int, int, int, int, byte *, int, int))re->DrawStretchRaw)(
        x, y, w, h, cinTable[handle].CIN_WIDTH, cinTable[handle].CIN_HEIGHT,
        buf, handle, cinTable[handle].dirty);
    cinTable[handle].dirty = 0;

    if (cinTable[handle].letterBox) {

        cls = (clientStatic_t *)imp_cls;
        barSize = (float)cls->vidConfig.height / 480.0f * 105.0f;
        barSizeCeil = (int)ceilf(barSize);
        xf = (float)x;
        wf = (float)w;
        material = cls->whiteMaterial;

        re = (refexport_t *)(void *)imp_re;
        ((void (*)(float, float, float, float, float, float, float, float, const vec_t *, int))re->DrawStretchPic)(
            xf, (float)y, wf, (float)(int)floorf(barSize),
            0.0f, 0.0f, 0.0f, 0.0f, clearColor, material);
        ((void (*)(float, float, float, float, float, float, float, float, const vec_t *, int))re->DrawStretchPic)(
            xf, (float)(y + h - barSizeCeil), wf, (float)barSizeCeil,
            0.0f, 0.0f, 0.0f, 0.0f, clearColor, material);
    }
done:;
}

void ROQ_DrawCinematic(void)
{
    if ((unsigned int)CL_handle <= 0xf) {
        ROQ_DrawCinematicFromHandle(CL_handle);
    }
}

static void RoQReset(void)
{
    int h;
    int fps;
    unsigned int t;

    h = currentHandle;
    if (h >= 0) {
        Sys_EndStreamedFile(cinTable[h].iFile);
        FS_FCloseFile(cinTable[h].iFile);
        cinTable[currentHandle].iFile = 0;
        FS_FOpenFileRead(cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, 1);
        Sys_BeginStreamedFile(cinTable[currentHandle].iFile, 0x10000);
        Sys_StreamedRead(cin.file, 0x10, 1, cinTable[currentHandle].iFile);

        t = CL_ScaledMilliseconds();
        cinTable[currentHandle].startTime = t;
        cinTable[currentHandle].lastTime = t;
        cinTable[currentHandle].lastFrameTime = t;
        cinTable[currentHandle].RoQPlayed = 0x18;

        fps = cin.file[6] | (cin.file[7] << 8);
        if (fps == 0) {
            fps = 30;
        }
        cinTable[currentHandle].roqFPS = fps;
        cinTable[currentHandle].numQuads = -1;
        cinTable[currentHandle].roq_id = cin.file[8] | (cin.file[9] << 8);
        cinTable[currentHandle].RoQFrameSize = cin.file[10] | (cin.file[11] << 8) | (cin.file[12] << 16);
        cinTable[currentHandle].roq_flags = cin.file[14] | (cin.file[15] << 8);
        cinTable[currentHandle].roqF0 = (signed char)cin.file[15];
        cinTable[currentHandle].roqF1 = (signed char)cin.file[14];
        cinTable[currentHandle].status = FMV_LOOPED;
    }
}

void ROQ_CloseAllVideos(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (cinTable[i].fileName[0] == '\0') {
            continue;
        }
        if (cinTable[i].status == FMV_EOF) {
            continue;
        }
        currentHandle = i;
        Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);
        if (!cinTable[currentHandle].buf) {
            continue;
        }
        if (!cinTable[currentHandle].alterGameState || *(int *)(void *)imp_clc == 1) {
            cinTable[currentHandle].status = FMV_EOF;
            RoQShutdown();
        }
    }
}

void ROQ_StopCinematic(void)
{
    int h;

    h = CL_handle;
    if ((unsigned int)h <= 0xf) {
        if (cinTable[h].status != FMV_EOF) {
            currentHandle = h;
            Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);
            if (cinTable[currentHandle].buf) {
                if (!cinTable[currentHandle].alterGameState || *(int *)(void *)imp_clc == 1) {
                    cinTable[currentHandle].status = FMV_EOF;
                    RoQShutdown();
                }
            }
        }
        SND_StopSounds(0);
        CL_handle = -1;
    }
}

extern void Com_Memcpy(void *dest, const void *src, int count);

e_status ROQ_RunCinematicFromHandle(int handle)
{
    unsigned int thisTime;
    unsigned int start;
    int h;
    unsigned int numFrames;
    byte *framedata;
    short samplesBuf[16384];

    if ((unsigned int)handle > 0xf || cinTable[handle].status == FMV_EOF) {
        return FMV_EOF;
    }

    if (cin.currentHandle != handle) {
        currentHandle = handle;
        cin.currentHandle = handle;
        cinTable[handle].status = FMV_EOF;
        RoQReset();
    }

    if (cinTable[handle].playonwalls < -1) {
        return cinTable[handle].status;
    }

    currentHandle = handle;

    if (cinTable[handle].alterGameState) {
        if (*(int *)(void *)imp_clc != 1) {
            return cinTable[handle].status;
        }
    }

    if (cinTable[handle].status == FMV_IDLE) {
        return (e_status)0;
    }

    thisTime = CL_ScaledMilliseconds();
    h = currentHandle;

    if (cinTable[h].material) {
        int elapsed = (int)thisTime - (int)cinTable[h].lastTime;
        float fElapsed;
        int absElapsed;
        if (elapsed >= 0) {
            fElapsed = (float)elapsed;
        } else {
            fElapsed = (float)(unsigned int)((unsigned int)thisTime - cinTable[h].lastTime);
        }
        absElapsed = (int)fElapsed;
        if (absElapsed < 0)
            absElapsed = -absElapsed;
        if (absElapsed > 100) {
            cinTable[h].startTime += (unsigned int)elapsed;
        }
    }

    handle = currentHandle;
    if (cinTable[handle].sound && ROQ_SoundEnabled()) {
        numFrames = 0;
    } else {
        numFrames = (unsigned int)((unsigned int)(thisTime - cinTable[handle].startTime) * (unsigned int)cinTable[handle].roqFPS) / 1000;
    }
    h = currentHandle;

    if ((long int)numFrames <= cinTable[h].tfps) {
        if ((thisTime - cinTable[h].lastFrameTime) * (unsigned int)cinTable[h].roqFPS > 4000) {
            cinTable[h].status = FMV_EOF;
        }
        goto frame_done;
    }
    cinTable[h].tfps = numFrames;
    cinTable[h].lastFrameTime = thisTime;

    h = currentHandle;
    start = cinTable[h].startTime;

frame_loop:
    h = currentHandle;
    if (cinTable[h].tfps == cinTable[h].numQuads) {
        goto frame_done_time;
    }

    if (cinTable[h].status != FMV_PLAY) {
        goto frame_done_time;
    }

    if (handle < 0) {
        goto after_parse;
    }

    Sys_StreamedRead(cin.file, cinTable[h].RoQFrameSize + 8, 1, cinTable[h].iFile);
    h = currentHandle;

    if (cinTable[h].RoQPlayed >= cinTable[h].ROQSize) {
        if (cinTable[h].holdAtEnd) {
            cinTable[h].status = FMV_EOF;
            handle = currentHandle;
            goto after_parse;
        }
        if (cinTable[h].looping) {
            RoQReset();
            h = currentHandle;
            handle = h;
            goto after_parse;
        } else {
            cinTable[h].status = FMV_EOF;
            handle = currentHandle;
            goto after_parse;
        }
    }

    framedata = cin.file;

parse_roq:
    h = currentHandle;
    {
        unsigned int roqId = cinTable[h].roq_id;
        if (roqId < 0x1001 || roqId > 0x1030) {
            cinTable[h].status = FMV_EOF;
            handle = h;
            goto do_packet_advance;
        }

        switch (roqId) {
        case 0x1020:
        {
            if (!cinTable[h].silent) {
                unsigned int frameSize = cinTable[h].RoQFrameSize;
                unsigned short roqFlags = (unsigned short)cinTable[h].roq_flags;

                if (frameSize > 0) {
                    unsigned int j;
                    short accum = roqFlags;
                    for (j = 0; j < frameSize; j++) {
                        accum += cin.sqrTable[framedata[j]];
                        samplesBuf[j * 2 + 1] = accum;
                        samplesBuf[j * 2] = accum;
                    }
                }
                SND_RawSamples(frameSize, 0x5622, 2, 1, (byte *)samplesBuf);
                h = currentHandle;
            }
            cinTable[h].sound = ROQ_SoundEnabled();
            handle = currentHandle;
            goto do_packet_advance;
        }

        case 0x1021:
        {
            if (!cinTable[h].silent) {
                unsigned int frameSize = cinTable[h].RoQFrameSize;
                unsigned short roqFlags = (unsigned short)cinTable[h].roq_flags;
                short left = (short)(roqFlags & 0xff00);
                short right = (short)((roqFlags & 0xff) << 8);

                if (frameSize > 0) {
                    unsigned int j;
                    for (j = 0; j < frameSize; j += 2) {
                        left += cin.sqrTable[framedata[j]];
                        right += cin.sqrTable[framedata[j + 1]];
                        samplesBuf[j] = left;
                        samplesBuf[j + 1] = right;
                    }
                }
                SND_RawSamples(frameSize >> 1, 0x5622, 2, 2, (byte *)samplesBuf);
                h = currentHandle;
            }
            cinTable[h].sound = ROQ_SoundEnabled();
            handle = currentHandle;
            goto do_packet_advance;
        }

        case 0x1011:
        {

            {
                void (*rendererCb)(void) = ((refexport_t *)(void *)imp_re)->SyncRenderThread;
                if (rendererCb)
                    rendererCb();
            }
            h = currentHandle;

            if (cinTable[h].numQuads & 1) {
                cinTable[h].normalBuffer0 = cinTable[h].t[1];
                RoQPrepMcomp(cinTable[h].roqF0, cinTable[h].roqF1);
                cinTable[h].VQ1(cin.qStatus[1], framedata);
                h = currentHandle;
                cinTable[h].buf = (byte *)&cin + cinTable[h].screenDelta;
            } else {
                cinTable[h].normalBuffer0 = cinTable[h].t[0];
                RoQPrepMcomp(cinTable[h].roqF0, cinTable[h].roqF1);
                cinTable[h].VQ0(cin.qStatus[0], framedata);
                h = currentHandle;
                cinTable[h].buf = (byte *)&cin;
            }

            {
                int numQ = cinTable[h].numQuads;
                if (numQ == 0) {

                    if (cinTable[h].numQuads == 0) {
                        Com_Memcpy((byte *)&cin + cinTable[h].screenDelta,
                                   (byte *)&cin,
                                   cinTable[h].samplesPerLine * cinTable[h].ysize);
                    }
                }
            }

            cinTable[h].numQuads++;
            cinTable[h].dirty = 1;
            handle = currentHandle;
            goto do_packet_advance;
        }

        case 0x1001:
        {
            if (cinTable[h].numQuads == -1) {

                if (h >= 0) {
                    cinTable[h].xsize = framedata[0] | (framedata[1] << 8);
                    cinTable[h].ysize = framedata[2] | (framedata[3] << 8);
                    cinTable[h].maxsize = framedata[4] | (framedata[5] << 8);
                    cinTable[h].minsize = framedata[6] | (framedata[7] << 8);
                    cinTable[h].CIN_HEIGHT = cinTable[h].ysize;
                    cinTable[h].CIN_WIDTH = cinTable[h].xsize;
                    cinTable[h].samplesPerLine = cinTable[h].CIN_WIDTH * cinTable[h].samplesPerPixel;
                    cinTable[h].screenDelta = cinTable[h].samplesPerLine * cinTable[h].CIN_HEIGHT;
                    cinTable[h].half = 0;
                    cinTable[h].smootheddouble = 0;
                    cinTable[h].VQ0 = cinTable[h].VQNormal;
                    cinTable[h].VQ1 = cinTable[h].VQBuffer;
                    cinTable[h].t[0] = cinTable[h].screenDelta;
                    cinTable[h].t[1] = -(cinTable[h].screenDelta);
                }

                if (cin.oldXOff == 0 && cin.oldYOff == 0 &&
                    cinTable[h].ysize == (unsigned int)cin.oldysize &&
                    cinTable[h].xsize == (unsigned int)cin.oldxsize) {

                } else {
                    int numCells, halfCells;
                    int sx, sy;

                    cin.oldXOff = 0;
                    cin.oldYOff = 0;
                    cin.oldysize = cinTable[h].ysize;
                    cin.oldxsize = cinTable[h].xsize;

                    numCells = (cinTable[h].xsize * cinTable[h].ysize) >> 4;
                    halfCells = numCells >> 2;

                    cinTable[h].onQuad = 0;

                    for (sy = 0; (unsigned int)sy < cinTable[h].ysize; sy += 16) {
                        for (sx = 0; (unsigned int)sx < cinTable[h].xsize; sx += 16) {
                            recurseQuad(sx, sy, 16, 0, 0);
                            h = currentHandle;
                        }
                    }

                    {
                        int sentStart = numCells + halfCells - 0x40 + 0x40;
                        int sentEnd = numCells + halfCells + 0x40;
                        int j;
                        for (j = sentStart; j < sentEnd; j++) {
                            cin.qStatus[0][j] = 0;
                            cin.qStatus[1][j] = 0;
                        }
                    }
                }

                thisTime = CL_ScaledMilliseconds();
                h = currentHandle;
                cinTable[h].lastTime = thisTime;
                cinTable[h].startTime = thisTime;
            }
            handle = currentHandle;
            {
                int hh = handle;
                cinTable[hh].numQuads++;
                if (cinTable[hh].numQuads == 0) {
                    cinTable[hh].numQuads = 0;
                }
                cinTable[hh].dirty = 1;
            }
            goto do_packet_advance;
        }

        case 0x1002:
        {
            unsigned short roqFlags = (unsigned short)cinTable[h].roq_flags;
            int numY, numCb;

            if ((roqFlags >> 8) == 0) {
                numY = 256;
            } else {
                numY = roqFlags >> 8;
            }
            numCb = roqFlags & 0xff;
            if (numCb == 0) {
                numCb = 256;
            }
            numCb *= 2;

            if (cinTable[h].half) {

                int bpp = cinTable[h].samplesPerPixel;
                if (bpp == 2) {

                } else if (bpp == 4) {

                    byte *input = framedata;
                    int idx;
                    for (idx = 0; idx < numY; idx++) {
                        unsigned char y[4], cb_val, cr_val;
                        long int *yy, *ub, *vr, *ug, *vg;
                        int p, q;
                        y[0] = input[0];
                        y[1] = input[1];
                        y[2] = input[2];
                        y[3] = input[3];
                        cb_val = input[4];
                        cr_val = input[5];
                        input += 6;

                        yy = ROQ_YY_tab;
                        ub = ROQ_UB_tab;
                        vr = ROQ_VR_tab;
                        ug = ROQ_UG_tab;
                        vg = ROQ_VG_tab;

                        for (p = 0; p < 4; p++) {
                            unsigned long pixel = yy[y[p]] | (ub[cb_val] << 8) | (vr[cr_val] << 16);
                            vq2[idx * 8 + p] = (unsigned short)pixel;
                        }
                    }
                    for (idx = 0; idx < numCb; idx++) {

                    }
                } else if (bpp == 1) {

                }
            } else if (cinTable[h].smootheddouble) {

                int bpp = cinTable[h].samplesPerPixel;
                if (bpp == 2) {

                } else if (bpp == 4) {

                    byte *input = framedata;
                    int idx;
                    for (idx = 0; idx < numY; idx++) {
                        input += 6;
                    }
                    for (idx = 0; idx < numCb; idx++) {

                    }
                }
            } else {
                int bpp = cinTable[h].samplesPerPixel;
                if (bpp == 2) {

                } else if (bpp == 4) {

                    {
                        byte *input = framedata;
                        int idx;
                        short unsigned int *vq2p = vq2;
                        for (idx = 0; idx < numY; idx++) {
                            unsigned char y0 = *input++;
                            unsigned char y1 = *input++;
                            unsigned char y2 = *input++;
                            unsigned char y3 = *input++;
                            unsigned char cbv = *input++;
                            unsigned char crv = *input++;

                            {
                                byte c0 = (byte)(ROQ_YY_tab[y0] + ROQ_UB_tab[cbv]);
                                byte c1 = (byte)(ROQ_YY_tab[y0] + ROQ_UG_tab[cbv] + ROQ_VG_tab[crv]);
                                byte c2 = (byte)(ROQ_YY_tab[y0] + ROQ_VR_tab[crv]);
                                byte c3 = (byte)(ROQ_YY_tab[y1] + ROQ_UB_tab[cbv]);
                                byte c4 = (byte)(ROQ_YY_tab[y1] + ROQ_UG_tab[cbv] + ROQ_VG_tab[crv]);
                                byte c5 = (byte)(ROQ_YY_tab[y1] + ROQ_VR_tab[crv]);
                                byte c6 = (byte)(ROQ_YY_tab[y2] + ROQ_UB_tab[cbv]);
                                byte c7 = (byte)(ROQ_YY_tab[y2] + ROQ_UG_tab[cbv] + ROQ_VG_tab[crv]);
                                byte c8 = (byte)(ROQ_YY_tab[y2] + ROQ_VR_tab[crv]);
                                byte c9 = (byte)(ROQ_YY_tab[y3] + ROQ_UB_tab[cbv]);
                                byte ca = (byte)(ROQ_YY_tab[y3] + ROQ_UG_tab[cbv] + ROQ_VG_tab[crv]);
                                byte cb2 = (byte)(ROQ_YY_tab[y3] + ROQ_VR_tab[crv]);
                                ((byte *)vq2p)[0] = c0;
                                ((byte *)vq2p)[1] = c1;
                                ((byte *)vq2p)[2] = c2;
                                ((byte *)vq2p)[3] = 255;
                                ((byte *)vq2p)[4] = c3;
                                ((byte *)vq2p)[5] = c4;
                                ((byte *)vq2p)[6] = c5;
                                ((byte *)vq2p)[7] = 255;
                                ((byte *)vq2p)[8] = c6;
                                ((byte *)vq2p)[9] = c7;
                                ((byte *)vq2p)[10] = c8;
                                ((byte *)vq2p)[11] = 255;
                                ((byte *)vq2p)[12] = c9;
                                ((byte *)vq2p)[13] = ca;
                                ((byte *)vq2p)[14] = cb2;
                                ((byte *)vq2p)[15] = 255;
                            }
                            vq2p += 8;
                        }

                        {
                            unsigned int *vq4p = (unsigned int *)vq4;
                            unsigned int *vq8p = (unsigned int *)vq8;
                            for (idx = 0; idx < numCb; idx++) {
                                unsigned int *cellA = (unsigned int *)vq2 + (*input++) * 4;
                                unsigned int *cellB = (unsigned int *)vq2 + (*input++) * 4;
                                int row;

                                for (row = 0; row < 2; row++) {
                                    RoQ_VQ2To4RGBA(&cellA, &cellB, &vq4p, &vq8p);
                                }
                            }
                        }
                    }
                } else if (bpp == 1) {

                    byte *input = framedata;
                    int idx;
                    for (idx = 0; idx < numY; idx++) {
                        input += 6;
                    }
                }
            }

            handle = currentHandle;
            goto do_packet_advance;
        }

        case 0x1013:
        {
            handle = currentHandle;
            goto do_packet_advance;
        }

        case 0x1030:
        {
            cinTable[h].status = FMV_EOF;
            handle = h;
            goto do_packet_advance;
        }

        default:
            cinTable[h].status = FMV_EOF;
            handle = h;
            goto do_packet_advance;
        }
    }

do_packet_advance:
    h = currentHandle;
    cinTable[h].RoQPlayed += cinTable[h].RoQFrameSize + 8;
    if (cinTable[h].status != FMV_EOF) {
        byte *nextHeader = framedata + cinTable[h].RoQFrameSize;
        cinTable[h].roq_id = nextHeader[0] | (nextHeader[1] << 8);
        cinTable[h].RoQFrameSize = nextHeader[2] | (nextHeader[3] << 8) | (nextHeader[4] << 16);
        cinTable[h].roq_flags = nextHeader[6] | (nextHeader[7] << 8);
        cinTable[h].roqF0 = (signed char)nextHeader[7];
        cinTable[h].roqF1 = (signed char)nextHeader[6];
    }
    handle = h;

after_parse:
    h = handle;
    if (cinTable[h].startTime != start) {
        thisTime = CL_ScaledMilliseconds();
        h = currentHandle;
        numFrames = (unsigned int)((unsigned int)(thisTime - cinTable[h].startTime) * (unsigned int)cinTable[h].roqFPS) / 1000;
        cinTable[h].tfps = numFrames;
        start = cinTable[h].startTime;
        h = currentHandle;
        if (cinTable[h].tfps != cinTable[h].numQuads) {
            if (cinTable[h].status == FMV_PLAY) {
                goto frame_loop;
            }
        }
    } else {
        h = currentHandle;
    }

frame_done_time:
    cinTable[h].lastTime = thisTime;

    if (cinTable[h].status == FMV_LOOPED) {
        cinTable[h].status = FMV_PLAY;
    }

    if (cinTable[h].status == FMV_EOF) {

    }

frame_done:
    h = currentHandle;
    return cinTable[h].status;
}

void ROQ_RunCinematic(void)
{
    if ((unsigned int)CL_handle <= 0xf) {
        ROQ_RunCinematicFromHandle(CL_handle);
    }
}

void ROQ_PlayCinematic_f(void)
{
    int argc;
    char *arg;
    char *argv2;
    int systemBits;
    int h;

    Com_DPrintf("CL_PlayCinematic_f\n");
    argc = *(int *)(void *)imp_clc;

    if (argc == 1) {

        h = CL_handle;
        if ((unsigned int)h <= 0xf) {
            if (cinTable[h].status != FMV_EOF) {
                currentHandle = h;
                Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);
                if (cinTable[currentHandle].buf) {
                    if (!cinTable[currentHandle].alterGameState || *(int *)(void *)imp_clc == 1) {
                        cinTable[currentHandle].status = FMV_EOF;
                        RoQShutdown();
                    }
                }
            }
            SND_StopSounds(0);
            CL_handle = -1;
        }
    } else if (argc == 2) {
        CL_StopLogo();
    }

    arg = Cmd_Argv(1);
    argv2 = Cmd_Argv(2);
    if (!argv2) {
        systemBits = 1;
    } else {
        switch (argv2[0]) {
        case '1':
            systemBits = 5;
            break;
        case '2':
            systemBits = 3;
            break;
        case '3':
            systemBits = 0x21;
            break;
        default:
            systemBits = 1;
            break;
        }
    }

    if (systemBits & 0x20) {
        CL_handle = ROQ_PlayCinematic(arg, 0, 0x69, 0x280, 0x10e, systemBits);
    } else {
        CL_handle = ROQ_PlayCinematic(arg, 0, 0, 0x280, 0x1e0, systemBits);
    }

    if (CL_handle >= 0) {
        SND_StopSounds(0);
        SND_FadeAllSounds(1.0f, 0);
        do {
            if ((unsigned int)CL_handle <= 0xf) {
                ROQ_RunCinematicFromHandle(CL_handle);
            }
            if (cinTable[currentHandle].buf) {
                break;
            }
        } while (cinTable[currentHandle].status == FMV_PLAY);
    }
}
