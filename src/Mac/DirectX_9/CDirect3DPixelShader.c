#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 textureUsageMask;
} CDirect3DPixelShaderImpl;

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 textureUsageMask;
    GLuint programId;
} COpenGLATITextFragmentShaderImpl;

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 textureUsageMask;
    UINT32 pad0xc;
    UINT32 stageCount;
} COpenGLNVidiaRegisterCombinersProgramImpl;

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 textureUsageMask;
    void *nameStr;
    void *codeStr;
    GLuint programId;
} COpenGLARBFragmentProgramImpl;
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DPixelShader_base[];
extern fnptr_t vtbl_CDirect3DPixelShader_ATI[];
extern fnptr_t vtbl_CDirect3DPixelShader_NVidia[];
extern fnptr_t vtbl_CDirect3DPixelShader_ARB[];

extern void *imp___ZN7COpenGL7sOpenGLE;
extern int g_fp_enable_count;
extern int g_fp_bind_count;

extern unsigned char *imp___ZN15CDirect3DDevice28mNeedsVertexShaderValidationE;
extern unsigned char *imp___ZN15CDirect3DDevice30mNeedsTransformationValidationE;
extern unsigned char *imp___ZN15CDirect3DDevice29mNeedsRasterizationValidationE;

static void CDirect3DPixelShader_InvalidateDeviceState(void)
{
    if (imp___ZN15CDirect3DDevice28mNeedsVertexShaderValidationE)
        *imp___ZN15CDirect3DDevice28mNeedsVertexShaderValidationE = 1;
    if (imp___ZN15CDirect3DDevice30mNeedsTransformationValidationE)
        *imp___ZN15CDirect3DDevice30mNeedsTransformationValidationE = 1;
    if (imp___ZN15CDirect3DDevice29mNeedsRasterizationValidationE)
        *imp___ZN15CDirect3DDevice29mNeedsRasterizationValidationE = 1;
}

ULONG CDirect3DPixelShader_AddRef(const CDirect3DPixelShader *_this);
HRESULT CDirect3DPixelShader_QueryInterface(const CDirect3DPixelShader *_this, const IID *iid, void **ppvObj);
ULONG CDirect3DPixelShader_Release(const CDirect3DPixelShader *_this);
HRESULT CDirect3DPixelShader_GetDevice(const CDirect3DPixelShader *_this, IDirect3DDevice9 **ppDevice);
HRESULT CDirect3DPixelShader_GetFunction(const CDirect3DPixelShader *_this, void *pDstData, UINT *pSizeOfData);

void ZN20CDirect3DPixelShaderD0Ev(const CDirect3DPixelShader *_this);
void ZN20CDirect3DPixelShaderD1Ev(const CDirect3DPixelShader *_this);
void ZN20CDirect3DPixelShaderD2Ev(const CDirect3DPixelShader *_this);

void COpenGLATITextFragmentShader_Enable(const COpenGLATITextFragmentShader *_this);
void COpenGLATITextFragmentShader_Disable(const COpenGLATITextFragmentShader *_this);
void COpenGLATITextFragmentShader_SetConstants(const COpenGLATITextFragmentShader *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber);
void COpenGLATITextFragmentShader_COpenGLATITextFragmentShader(const COpenGLATITextFragmentShader *_this, const void *pSrcData);
void ZN28COpenGLATITextFragmentShaderD0Ev(const COpenGLATITextFragmentShader *_this);
void ZN28COpenGLATITextFragmentShaderD1Ev(const COpenGLATITextFragmentShader *_this);

void COpenGLNVidiaRegisterCombinersProgram_Enable(const COpenGLNVidiaRegisterCombinersProgram *_this);
void COpenGLNVidiaRegisterCombinersProgram_Disable(const COpenGLNVidiaRegisterCombinersProgram *_this);
void COpenGLNVidiaRegisterCombinersProgram_SetConstants(const COpenGLNVidiaRegisterCombinersProgram *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber);
void COpenGLNVidiaRegisterCombinersProgram_COpenGLNVidiaRegisterCombinersProgram(const COpenGLNVidiaRegisterCombinersProgram *_this, const void *pSrcData);
void ZN37COpenGLNVidiaRegisterCombinersProgramD0Ev(const COpenGLNVidiaRegisterCombinersProgram *_this);
void ZN37COpenGLNVidiaRegisterCombinersProgramD1Ev(const COpenGLNVidiaRegisterCombinersProgram *_this);
void ZN37COpenGLNVidiaRegisterCombinersProgramD2Ev(const COpenGLNVidiaRegisterCombinersProgram *_this);

void COpenGLARBFragmentProgram_Enable(const COpenGLARBFragmentProgram *_this);
void COpenGLARBFragmentProgram_Disable(const COpenGLARBFragmentProgram *_this);
void COpenGLARBFragmentProgram_SetConstants(const COpenGLARBFragmentProgram *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber);
void COpenGLARBFragmentProgram_COpenGLARBFragmentProgram(const COpenGLARBFragmentProgram *_this, const string *Name, const string *Code);
void ZN25COpenGLARBFragmentProgramD0Ev(const COpenGLARBFragmentProgram *_this);
void ZN25COpenGLARBFragmentProgramD1Ev(const COpenGLARBFragmentProgram *_this);
void ZN25COpenGLARBFragmentProgramD2Ev(const COpenGLARBFragmentProgram *_this);

ULONG CDirect3DPixelShader_AddRef(const CDirect3DPixelShader *_this)
{
    CDirect3DPixelShaderImpl *shader;

    shader = (CDirect3DPixelShaderImpl *)_this;
    ++shader->refCount;
    return shader->refCount;
}

HRESULT CDirect3DPixelShader_QueryInterface(const CDirect3DPixelShader *_this, const IID *iid, void **ppvObj)
{
    (void)iid;

    *ppvObj = (void *)_this;
    CDirect3DPixelShader_AddRef(_this);
    return 0;
}

ULONG CDirect3DPixelShader_Release(const CDirect3DPixelShader *_this)
{
    CDirect3DPixelShaderImpl *shader;
    ULONG refCount;

    shader = (CDirect3DPixelShaderImpl *)_this;
    refCount = --shader->refCount;
    if (!refCount) {

        void **vtable = shader->vtable;
        ((void (*)(const CDirect3DPixelShader *))vtable[6])(_this);
    }

    return refCount;
}

void ZN20CDirect3DPixelShaderD2Ev(const CDirect3DPixelShader *_this)
{
    CDirect3DPixelShaderImpl *shader = (CDirect3DPixelShaderImpl *)_this;
    shader->vtable = vtbl_CDirect3DPixelShader_base;
    CDirect3DPixelShader_InvalidateDeviceState();
}

void ZN20CDirect3DPixelShaderD1Ev(const CDirect3DPixelShader *_this)
{
    ZN20CDirect3DPixelShaderD2Ev(_this);
}

void ZN20CDirect3DPixelShaderD0Ev(const CDirect3DPixelShader *_this)
{
    ZN20CDirect3DPixelShaderD2Ev(_this);
    free((void *)_this);
}

void COpenGLATITextFragmentShader_Enable(const COpenGLATITextFragmentShader *_this)
{
    (void)_this;

    return 0;
}

void COpenGLATITextFragmentShader_Disable(const COpenGLATITextFragmentShader *_this)
{
    (void)_this;

    return 0;
}

void COpenGLATITextFragmentShader_SetConstants(const COpenGLATITextFragmentShader *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber)
{
    (void)_this;
    (void)Register;
    (void)pConstantData;
    (void)Vector4fCount;
    (void)CommandNumber;

    return 0;
}

void COpenGLATITextFragmentShader_COpenGLATITextFragmentShader(const COpenGLATITextFragmentShader *_this, const void *pSrcData)
{
    COpenGLATITextFragmentShaderImpl *shader;

    (void)pSrcData;

    shader = (COpenGLATITextFragmentShaderImpl *)_this;
    shader->vtable = vtbl_CDirect3DPixelShader_ATI;
    shader->refCount = 1;
    shader->textureUsageMask = 0;
    shader->programId = 0;

    return 0;
}

static void COpenGLATITextFragmentShader_DestroyImpl(const COpenGLATITextFragmentShader *_this)
{
    COpenGLATITextFragmentShaderImpl *shader = (COpenGLATITextFragmentShaderImpl *)_this;
    shader->vtable = vtbl_CDirect3DPixelShader_ATI;

    ZN20CDirect3DPixelShaderD2Ev((const CDirect3DPixelShader *)_this);
}

void ZN28COpenGLATITextFragmentShaderD1Ev(const COpenGLATITextFragmentShader *_this)
{
    COpenGLATITextFragmentShader_DestroyImpl(_this);
}

void ZN28COpenGLATITextFragmentShaderD0Ev(const COpenGLATITextFragmentShader *_this)
{
    COpenGLATITextFragmentShader_DestroyImpl(_this);
    free((void *)_this);
}

void COpenGLNVidiaRegisterCombinersProgram_Enable(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    (void)_this;
    return 0;
}

void COpenGLNVidiaRegisterCombinersProgram_Disable(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    (void)_this;

    return 0;
}

void COpenGLNVidiaRegisterCombinersProgram_SetConstants(const COpenGLNVidiaRegisterCombinersProgram *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber)
{
    (void)_this;
    (void)Register;
    (void)pConstantData;
    (void)Vector4fCount;
    (void)CommandNumber;

    return 0;
}

void COpenGLNVidiaRegisterCombinersProgram_COpenGLNVidiaRegisterCombinersProgram(const COpenGLNVidiaRegisterCombinersProgram *_this, const void *pSrcData)
{
    COpenGLNVidiaRegisterCombinersProgramImpl *program;

    program = (COpenGLNVidiaRegisterCombinersProgramImpl *)_this;
    program->vtable = vtbl_CDirect3DPixelShader_NVidia;
    program->refCount = 1;
    program->textureUsageMask = 0;
    program->pad0xc = (UINT32)(unsigned long)pSrcData;
    program->stageCount = 0;
    return 0;
}

static void COpenGLNVidiaRegisterCombinersProgram_DestroyImpl(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    COpenGLNVidiaRegisterCombinersProgramImpl *program;
    program = (COpenGLNVidiaRegisterCombinersProgramImpl *)_this;
    program->vtable = vtbl_CDirect3DPixelShader_NVidia;

    ZN20CDirect3DPixelShaderD2Ev((const CDirect3DPixelShader *)_this);
}

void ZN37COpenGLNVidiaRegisterCombinersProgramD2Ev(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    COpenGLNVidiaRegisterCombinersProgram_DestroyImpl(_this);
}

void ZN37COpenGLNVidiaRegisterCombinersProgramD1Ev(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    COpenGLNVidiaRegisterCombinersProgram_DestroyImpl(_this);
}

void ZN37COpenGLNVidiaRegisterCombinersProgramD0Ev(const COpenGLNVidiaRegisterCombinersProgram *_this)
{
    COpenGLNVidiaRegisterCombinersProgram_DestroyImpl(_this);
    free((void *)_this);
}

void COpenGLARBFragmentProgram_Enable(const COpenGLARBFragmentProgram *_this)
{
    COpenGLARBFragmentProgramImpl *program = (COpenGLARBFragmentProgramImpl *)_this;
    if (program->programId) {
        glEnable(0x8804);
        glBindProgramARB(0x8804, program->programId);
    }
    return 0;
}

void COpenGLARBFragmentProgram_Disable(const COpenGLARBFragmentProgram *_this)
{
    (void)_this;
    glDisable(0x8804);
    glBindProgramARB(0x8804, 0);
    return 0;
}

void COpenGLARBFragmentProgram_SetConstants(const COpenGLARBFragmentProgram *_this, UINT32 Register, const float *pConstantData, UINT32 Vector4fCount, UINT32 CommandNumber)
{
    UINT32 i;
    const float *pf = pConstantData;
    (void)_this;
    (void)CommandNumber;
    for (i = 0; i < Vector4fCount; i++) {
#ifndef __EMSCRIPTEN__
        glProgramEnvParameter4fvARB(0x8804, Register + i, pf);
#endif
        pf += 4;
    }
    return 0;
}

void COpenGLARBFragmentProgram_COpenGLARBFragmentProgram(const COpenGLARBFragmentProgram *_this, const string *Name, const string *Code)
{
#ifdef __EMSCRIPTEN__
    extern void Com_Error(int, const char *, ...);
    Com_Error(1, "ARB shaders are unsupported by the WebGL fixed-function target");
    return ;
#else

    COpenGLARBFragmentProgramImpl *program;
    const char *codeStr;
    int codeLen;

    program = (COpenGLARBFragmentProgramImpl *)_this;
    program->vtable = vtbl_CDirect3DPixelShader_ARB;
    program->refCount = 1;
    program->textureUsageMask = 0;
    program->nameStr = NULL;
    program->codeStr = NULL;
    program->programId = 0;

    codeStr = *(const char **)Code;
    if (!codeStr || !codeStr[0])
        return 0;

    codeLen = strlen(codeStr);

    {
        const char *p = codeStr;
        while ((p = strstr(p, "texture[")) != NULL) {
            p += 8;
            while (*p == ' ')
                p++;
            if (*p >= '0' && *p <= '9') {
                int idx = *p - '0';
                if (idx < 16)
                    program->textureUsageMask |= (1u << idx);
            }
            p++;
        }
    }

    glGenProgramsARB(1, &program->programId);
    glBindProgramARB(0x8804, program->programId);
    glProgramStringARB(0x8804, 0x8875 ,
                       codeLen, codeStr);

    {
        int errorPos = 0;
        glGetIntegerv(0x864B , &errorPos);
        if (errorPos != -1) {
            fprintf(stderr, "[ARB FP] Compile error at position %d in program %u\n",
                    errorPos, program->programId);
        }
    }

    glBindProgramARB(0x8804, 0);

    return 0;
#endif
}

static void COpenGLARBFragmentProgram_DestroyImpl(const COpenGLARBFragmentProgram *_this)
{
    COpenGLARBFragmentProgramImpl *program;
    program = (COpenGLARBFragmentProgramImpl *)_this;
    program->vtable = vtbl_CDirect3DPixelShader_ARB;
    if (program->programId) {
#ifndef __EMSCRIPTEN__
        glDeleteProgramsARB(1, &program->programId);
#endif
        program->programId = 0;
    }
    ZN20CDirect3DPixelShaderD2Ev((const CDirect3DPixelShader *)_this);
}

void ZN25COpenGLARBFragmentProgramD2Ev(const COpenGLARBFragmentProgram *_this)
{
    COpenGLARBFragmentProgram_DestroyImpl(_this);
}

void ZN25COpenGLARBFragmentProgramD1Ev(const COpenGLARBFragmentProgram *_this)
{
    COpenGLARBFragmentProgram_DestroyImpl(_this);
}

void ZN25COpenGLARBFragmentProgramD0Ev(const COpenGLARBFragmentProgram *_this)
{
    COpenGLARBFragmentProgram_DestroyImpl(_this);
    free((void *)_this);
}

HRESULT CDirect3DPixelShader_GetDevice(const CDirect3DPixelShader *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    *ppDevice = NULL;
    return 0;
}

HRESULT CDirect3DPixelShader_GetFunction(const CDirect3DPixelShader *_this, void *pDstData, UINT *pSizeOfData)
{
    (void)_this;
    (void)pDstData;
    *pSizeOfData = 0;
    return 0;
}

fnptr_t vtbl_CDirect3DPixelShader_base[] = { (fnptr_t)CDirect3DPixelShader_QueryInterface, (fnptr_t)CDirect3DPixelShader_AddRef, (fnptr_t)CDirect3DPixelShader_Release, (fnptr_t)CDirect3DPixelShader_GetDevice, (fnptr_t)CDirect3DPixelShader_GetFunction, (fnptr_t)ZN20CDirect3DPixelShaderD1Ev, (fnptr_t)ZN20CDirect3DPixelShaderD0Ev };
fnptr_t vtbl_CDirect3DPixelShader_ATI[] = { (fnptr_t)CDirect3DPixelShader_QueryInterface, (fnptr_t)CDirect3DPixelShader_AddRef, (fnptr_t)CDirect3DPixelShader_Release, (fnptr_t)CDirect3DPixelShader_GetDevice, (fnptr_t)CDirect3DPixelShader_GetFunction, (fnptr_t)ZN28COpenGLATITextFragmentShaderD1Ev, (fnptr_t)ZN28COpenGLATITextFragmentShaderD0Ev, (fnptr_t)COpenGLATITextFragmentShader_Enable, (fnptr_t)COpenGLATITextFragmentShader_Disable, (fnptr_t)COpenGLATITextFragmentShader_SetConstants };
fnptr_t vtbl_CDirect3DPixelShader_NVidia[] = { (fnptr_t)CDirect3DPixelShader_QueryInterface, (fnptr_t)CDirect3DPixelShader_AddRef, (fnptr_t)CDirect3DPixelShader_Release, (fnptr_t)CDirect3DPixelShader_GetDevice, (fnptr_t)CDirect3DPixelShader_GetFunction, (fnptr_t)ZN37COpenGLNVidiaRegisterCombinersProgramD1Ev, (fnptr_t)ZN37COpenGLNVidiaRegisterCombinersProgramD0Ev, (fnptr_t)COpenGLNVidiaRegisterCombinersProgram_Enable, (fnptr_t)COpenGLNVidiaRegisterCombinersProgram_Disable, (fnptr_t)COpenGLNVidiaRegisterCombinersProgram_SetConstants };
fnptr_t vtbl_CDirect3DPixelShader_ARB[] = { (fnptr_t)CDirect3DPixelShader_QueryInterface, (fnptr_t)CDirect3DPixelShader_AddRef, (fnptr_t)CDirect3DPixelShader_Release, (fnptr_t)CDirect3DPixelShader_GetDevice, (fnptr_t)CDirect3DPixelShader_GetFunction, (fnptr_t)ZN25COpenGLARBFragmentProgramD1Ev, (fnptr_t)ZN25COpenGLARBFragmentProgramD0Ev, (fnptr_t)COpenGLARBFragmentProgram_Enable, (fnptr_t)COpenGLARBFragmentProgram_Disable, (fnptr_t)COpenGLARBFragmentProgram_SetConstants };
