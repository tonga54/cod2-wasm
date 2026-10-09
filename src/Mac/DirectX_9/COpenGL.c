#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_COpenGL[];
extern fnptr_t vtbl_COpenGLVertexProgram[];

unsigned char __attribute__((visibility("default")))
COpenGL_sOpenGL[4096] = { 0 };

void *imp__ZN7COpenGL7sOpenGLE = COpenGL_sOpenGL;

#define COPENGL_TEXUNIT_STRIDE 304
#define COPENGL_MAX_TEXUNITS 16
static unsigned char COpenGL_texUnitState[COPENGL_MAX_TEXUNITS * COPENGL_TEXUNIT_STRIDE];

unsigned char *COpenGL_TexUnitBase(const COpenGL *_this, UINT32 Unit)
{
    unsigned char *p = (unsigned char *)_this;

    if (*(void **)(p + 1628) == 0)
        *(void **)(p + 1628) = COpenGL_texUnitState;
    return (unsigned char *)(*(void **)(p + 1628)) + Unit * COPENGL_TEXUNIT_STRIDE;
}

UINT32 COpenGL_sFrameCount = 0;
bool COpenGL_sDrawFlag = 0;
GLuint COpenGL_sShowTextureID = 0;
GLint g_VAOID = 0;

unsigned int COpenGL_SetColorMaterialEnable(const COpenGL *_this, int Value)
{
    (void)_this;
    (void)Value;
    return 0;
}

unsigned int COpenGL_SetActiveTexUnit(const COpenGL *_this, UINT32 Unit)
{
    unsigned char *p = (unsigned char *)_this;
    if (*(int *)(p + 4) != (int)Unit) {
        *(int *)(p + 4) = (int)Unit;
        glActiveTextureARB(Unit + 0x84C0 );
    }
    return 0;
}

unsigned int COpenGL_SetDither(const COpenGL *_this, int Value)
{
    (void)_this;
    (void)Value;
    return 0;
}

unsigned int COpenGL_SetBlendEXT(const COpenGL *_this, int ForceValidation,
                                 GLenum SrcFactorRGB, GLenum DstFactorRGB, GLenum SrcFactorAlpha, GLenum DstFactorAlpha)
{
    unsigned char *p = (unsigned char *)_this;
    if (ForceValidation == 0 &&
        SrcFactorRGB == *(int *)(p + 2084) && DstFactorRGB == *(int *)(p + 2088) &&
        SrcFactorAlpha == *(int *)(p + 2092) && DstFactorAlpha == *(int *)(p + 2096))
        return 0;
    *(int *)(p + 2084) = SrcFactorRGB;
    *(int *)(p + 2088) = DstFactorRGB;
    *(int *)(p + 2092) = SrcFactorAlpha;
    *(int *)(p + 2096) = DstFactorAlpha;
    glBlendFuncSeparateEXT(SrcFactorRGB, DstFactorRGB, SrcFactorAlpha, DstFactorAlpha);
    return 0;
}

unsigned int COpenGL_SetBlend(const COpenGL *_this, int ForceValidation,
                              GLenum SrcFactorRGB, GLenum DstFactorRGB)
{
    unsigned char *p = (unsigned char *)_this;
    if (ForceValidation == 0 &&
        SrcFactorRGB == *(int *)(p + 2084) && DstFactorRGB == *(int *)(p + 2088))
        return 0;
    *(int *)(p + 2084) = SrcFactorRGB;
    *(int *)(p + 2088) = DstFactorRGB;
    glBlendFunc(SrcFactorRGB, DstFactorRGB);
    return 0;
}

unsigned int COpenGL_SetVertexProgram(const COpenGL *_this, const COpenGLVertexProgram *pOGLVertexProgramInfo)
{
    (void)_this;
    (void)pOGLVertexProgramInfo;
    return 0;
}

unsigned int COpenGL_SetVARInfo(const COpenGL *_this, const unsigned int *pVAStart, UINT32 Size)
{
    (void)_this;
    (void)pVAStart;
    (void)Size;
    return 0;
}

unsigned int COpenGL_DisableVertexProgramStream(const COpenGL *_this, UINT32 AttribArray)
{
    (void)_this;
    (void)AttribArray;
    return 0;
}

unsigned int COpenGL_CTexUnit_SetTex(const CTexUnit *_this, GLenum Target, const COpenGLTexture *pOpenGLTexInfo)
{
    (void)_this;
    (void)Target;
    (void)pOpenGLTexInfo;
    return 0;
}

unsigned int COpenGL_SetTex(const COpenGL *_this, UINT32 Unit, GLenum Target, const COpenGLTexture *pOpenGLTexInfo)
{
    (void)_this;
    (void)Unit;
    (void)Target;
    (void)pOpenGLTexInfo;
    return 0;
}

unsigned int COpenGL_SetVertexProgramStreamInfo(const COpenGL *_this, UINT32 AttribArray,
                                                GLint VSize, GLenum VType, int Normalized, GLsizei Stride, const unsigned int *pStream)
{
    (void)_this;
    (void)AttribArray;
    (void)VSize;
    (void)VType;
    (void)Normalized;
    (void)Stride;
    (void)pStream;
    return 0;
}

unsigned int COpenGL_CTexUnit_Reset(const CTexUnit *_this, int SupportsAnisotropicFiltering,
                                    int SupportsLODBias, int IsProgramableOnly)
{
    (void)_this;
    (void)SupportsAnisotropicFiltering;
    (void)SupportsLODBias;
    (void)IsProgramableOnly;
    return 0;
}

unsigned int COpenGL_SetVAO(const COpenGL *_this, const COpenGLVAO *VAO,
                            int IsFixedFunction, int ForceValidation)
{
    (void)_this;
    (void)VAO;
    (void)IsFixedFunction;
    (void)ForceValidation;
    return 0;
}

unsigned int COpenGL_EnableColorArray(const COpenGL *_this, GLint Size, GLenum Type,
                                      GLsizei Stride, const unsigned int *Pointer)
{
    (void)_this;
    (void)Size;
    (void)Type;
    (void)Stride;
    (void)Pointer;
    return 0;
}

unsigned int COpenGL_DisableColorArray(const COpenGL *_this)
{
    (void)_this;
    return 0;
}

unsigned int COpenGL_EnableNormalArray(const COpenGL *_this, GLenum Type, GLsizei Stride, const unsigned int *Pointer)
{
    (void)_this;
    (void)Type;
    (void)Stride;
    (void)Pointer;
    return 0;
}

unsigned int COpenGL_DisableNormalArray(const COpenGL *_this)
{
    (void)_this;
    return 0;
}

unsigned int COpenGL_EnableVertexArray(const COpenGL *_this, GLint Size, GLenum Type,
                                       GLsizei Stride, const unsigned int *Pointer)
{
    (void)_this;
    (void)Size;
    (void)Type;
    (void)Stride;
    (void)Pointer;
    return 0;
}

unsigned int COpenGL_SetLight(const COpenGL *_this, UINT32 Light, const LightInfoType *Src)
{
    (void)_this;
    (void)Light;
    (void)Src;
    return 0;
}

unsigned int COpenGL_EnableTexCoordArray(const COpenGL *_this, UINT32 Unit, GLint Size,
                                         GLenum Type, GLsizei Stride, const unsigned int *Pointer)
{
    (void)_this;
    (void)Unit;
    (void)Size;
    (void)Type;
    (void)Stride;
    (void)Pointer;
    return 0;
}

unsigned int COpenGL_DisableTexCoordArray(const COpenGL *_this, UINT32 Unit)
{
    (void)_this;
    (void)Unit;
    return 0;
}

unsigned int COpenGL_SetTexMatrix(const COpenGL *_this, UINT32 Unit, const float *m)
{
    (void)_this;
    (void)Unit;
    (void)m;
    return 0;
}

unsigned int COpenGL_SetTexMatrixIdentity(const COpenGL *_this, UINT32 Unit)
{
    (void)_this;
    (void)Unit;
    return 0;
}

unsigned int COpenGL_Shutdown(const COpenGL *_this)
{
    (void)_this;
    return 0;
}

unsigned int COpenGL_SetTexGenEnable(const COpenGL *_this, UINT32 Unit,
                                     int Enable, int EnableReflection, int EnableNormalMap)
{
    (void)_this;
    (void)Unit;
    (void)Enable;
    (void)EnableReflection;
    (void)EnableNormalMap;
    return 0;
}

unsigned int COpenGL_DisableTexUnit(const COpenGL *_this, UINT32 Unit)
{
    (void)_this;
    (void)Unit;
    return 0;
}

unsigned int COpenGL_ReleaseVAOBinding(const COpenGL *_this, const GLuint *TexID)
{
    (void)_this;
    (void)TexID;
    return 0;
}

unsigned int COpenGL_ReleaseTextureBinding(const COpenGL *_this, const GLuint *TexID)
{
    (void)_this;
    (void)TexID;
    return 0;
}

unsigned int COpenGL_Init(const COpenGL *_this, UINT32 Buffers, UINT32 MaxTextureUnits, UINT32 MaxTextureImageUnits)
{
    (void)_this;
    (void)Buffers;
    (void)MaxTextureUnits;
    (void)MaxTextureImageUnits;
    return 0;
}

unsigned int COpenGL_COpenGL(const COpenGL *_this)
{
    (void)_this;
    return 0;
}

void ZN7COpenGLD1Ev(void *_this)
{
    (void)_this;
}

void ZN7COpenGLD0Ev(void *_this)
{
    (void)_this;
}

void ZN7COpenGLD2Ev(void *_this)
{
    (void)_this;
}

static void GLOBAL__D__ZN7COpenGL7sOpenGLE(void)
{

}

#ifdef _MSC_VER
COD2_CONSTRUCTOR(GLOBAL__I__ZN7COpenGL7sOpenGLE)
#else
__attribute__((constructor)) void GLOBAL__I__ZN7COpenGL7sOpenGLE(void)
#endif
{

    COpenGL_COpenGL((const COpenGL *)COpenGL_sOpenGL);
}

unsigned int COpenGLVertexProgram_COpenGLVertexProgram(const COpenGLVertexProgram *_this, const char *pSrcData)
{
#ifdef __EMSCRIPTEN__
    extern void Com_Error(int, const char *, ...);
    Com_Error(1, "ARB shaders are unsupported by the WebGL fixed-function target");
    return 0;
#else


    GLuint *progIdPtr = (GLuint *)_this;

    if (!pSrcData || pSrcData[0] != '!' || pSrcData[1] != '!') {
        *progIdPtr = 0;
        return 0;
    }

    {
        int len = strlen(pSrcData);
        int errorPos = -1;

        glGenProgramsARB(1, progIdPtr);
        glBindProgramARB(0x8620 , *progIdPtr);
        glProgramStringARB(0x8620, 0x8875 ,
                           len, pSrcData);

        glGetIntegerv(0x864B , &errorPos);
        if (errorPos != -1) {
            fprintf(stderr, "[ARB VP] Compile error at position %d in program %u\n",
                    errorPos, *progIdPtr);
        }

        glBindProgramARB(0x8620, 0);
    }

    return 0;
#endif
}

void ZN20COpenGLVertexProgramD1Ev(void *_this)
{
    (void)_this;
}

void ZN20COpenGLVertexProgramD0Ev(void *_this)
{
    (void)_this;
}

void ZN20COpenGLVertexProgramD2Ev(void *_this)
{
    (void)_this;
}

void ZN7CBaseVAD1Ev(void *_this)
{
    (void)_this;
}
void ZN7CBaseVAD0Ev(void *_this)
{
    (void)_this;
}

unsigned int CColorArray_Enable(const CColorArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CColorArray_Disable(const CColorArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CSecondaryColorArray_Enable(const CSecondaryColorArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CSecondaryColorArray_Disable(const CSecondaryColorArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CNormalArray_Enable(const CNormalArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CNormalArray_Disable(const CNormalArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CVertexArray_Enable(const CVertexArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CVertexArray_Disable(const CVertexArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CTexCoordArray_Enable(const CTexCoordArray *_this)
{
    (void)_this;
    return 0;
}
unsigned int CTexCoordArray_Disable(const CTexCoordArray *_this)
{
    (void)_this;
    return 0;
}

void ZN14CTexCoordArrayD1Ev(void *_this)
{
    (void)_this;
}
void ZN14CTexCoordArrayD0Ev(void *_this)
{
    (void)_this;
}
void ZN11CColorArrayD0Ev(void *_this)
{
    (void)_this;
}
void ZN11CColorArrayD1Ev(void *_this)
{
    (void)_this;
}
void ZN20CSecondaryColorArrayD0Ev(void *_this)
{
    (void)_this;
}
void ZN20CSecondaryColorArrayD1Ev(void *_this)
{
    (void)_this;
}
void ZN12CVertexArrayD0Ev(void *_this)
{
    (void)_this;
}
void ZN12CVertexArrayD1Ev(void *_this)
{
    (void)_this;
}
void ZN12CNormalArrayD0Ev(void *_this)
{
    (void)_this;
}
void ZN12CNormalArrayD1Ev(void *_this)
{
    (void)_this;
}

void ZNSt4listIPmSaIS0_EE5eraseESt14_List_iteratorIS0_E(void) {}
void ZNSt6vectorIN7COpenGL8CTexUnitESaIS1_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS1_S3_EERKS1_(void) {}
void ZNSt6vectorIN7COpenGL8CTexUnitESaIS1_EE5eraseEN9__gnu_cxx17__normal_iteratorIPS1_S3_EES7_(void) {}

fnptr_t vtbl_COpenGL[] = { (fnptr_t)ZN7COpenGLD1Ev, (fnptr_t)ZN7COpenGLD0Ev };
fnptr_t vtbl_COpenGLVertexProgram[] = { (fnptr_t)ZN20COpenGLVertexProgramD1Ev, (fnptr_t)ZN20COpenGLVertexProgramD0Ev };
