#include "GL/glc.h"
#include <stdio.h>

/* This test checks that for routines that are not "global commands", GLC issues GLC_STATE_ERROR if no
 * context has been made current to the issuing thread.
 */

#define TestErrorCode(func) \
    err = glcGetError(); \
    if (err != GLC_STATE_ERROR) { \
        printf(##func"() Unexpected error : 0x%x\n", (int)err); \
        return -1; \
    } \
    err = glcGetError(); \
    if (err) { \
        printf("Unexpected error : 0x%x\n", (int)err); \
        return -1; \
    }

int main(void)
{
    GLCenum err;

    err = glcGetError();
    if (err) {
        printf("Unexpected error : 0x%x\n", (int)err);
        return -1;
    }

    /* Context commands */

    glcCallbackFunc(GLC_OP_glcUnmappedCode, NULL);
    TestErrorCode("glcCallbackFunc");

    glcDataPointer(NULL);
    TestErrorCode("glcDataPointer");

    glcDeleteGLObjects();
    TestErrorCode("glcDeleteGLObjects");

    glcDisable(GLC_MIPMAP);
    TestErrorCode("glcDisable");

    glcEnable(GLC_GL_OBJECTS);
    TestErrorCode("glcEnable");

    glcGetCallbackFunc(GLC_OP_glcUnmappedCode);
    TestErrorCode("glcGetCallbackFunc");

    glcGetListc(GLC_CATALOG_LIST, 0);
    TestErrorCode("glcGetListc");

    glcGetListi(GLC_FONT_LIST, 0);
    TestErrorCode("glcGetListi");

    glcGetPointer(GLC_DATA_POINTER);
    TestErrorCode("glcGetPointer");

    glcGetc(GLC_RELEASE);
    TestErrorCode("glcGetc");

    glcGetf(GLC_RESOLUTION);
    TestErrorCode("glcGetf");

    glcGetfv(GLC_BITMAP_MATRIX, NULL);
    TestErrorCode("glcGetfv");

    glcGeti(GLC_VERSION_MAJOR);
    TestErrorCode("glcGeti");

    glcIsEnabled(GLC_AUTO_FONT);
    TestErrorCode("glcIsEnabled");

    glcStringType(GLC_UCS1);
    TestErrorCode("glcStringType");

    /* Master commands */

    glcAppendCatalog(NULL);
    TestErrorCode("glcAppendCatalog");

    glcGetMasterListc(0, GLC_CHAR_LIST, 0);
    TestErrorCode("glcGetMasterListc");

    glcGetMasterMap(0,0);
    TestErrorCode("glcGetMasterMap");

    glcGetMasterc(0, GLC_FAMILY);
    TestErrorCode("glcGetMasterc");

    glcGetMasteri(0, GLC_CHAR_COUNT);
    TestErrorCode("glcGetMasteri");

    glcPrependCatalog(NULL);
    TestErrorCode("glcPrependCatalog");

    glcRemoveCatalog(0);
    TestErrorCode("glcRemoveCatalog");

    /* Font commands */

    glcAppendFont(0);
    TestErrorCode("glcAppendFont");

    glcDeleteFont(0);
    TestErrorCode("glcDeleteFont");

    glcFont(0);
    TestErrorCode("glcFont");

    glcFontFace(0, NULL);
    TestErrorCode("glcFontFace");

    glcFontMap(0, 0, NULL);
    TestErrorCode("glcFontMap");

    glcGenFontID();
    TestErrorCode("glcGenFontID");

    glcGetFontFace(0);
    TestErrorCode("glcGetFontFace");

    glcGetFontListc(0, GLC_CHAR_LIST, 0);
    TestErrorCode("glcGetFontListc");

    glcGetFontMap(0, 0);
    TestErrorCode("glcGetFontMap");

    glcGetFontc(0, GLC_FAMILY);
    TestErrorCode("glcGetFontc");

    glcGetFonti(0, GLC_IS_FIXED_PITCH);
    TestErrorCode("glcGetFonti");

    glcIsFont(0);
    TestErrorCode("glcIsFont");

    glcNewFontFromFamily(1, NULL);
    TestErrorCode("glcNewFontFromFamily");

    glcNewFontFromMaster(1, 0);
    TestErrorCode("glcNewFontFromMaster");

    /* Transformation commands */

    glcLoadIdentity();
    TestErrorCode("glcLoadIdentity");

    glcLoadMatrix(NULL);
    TestErrorCode("glcLoadMatrix");

    glcMultMatrix(NULL);
    TestErrorCode("glcMultMatrix");

    glcRotate(0.);
    TestErrorCode("glcRotate");

    glcScale(1., 1.);
    TestErrorCode("glcScale");

    /* Rendering commands */

    glcRenderChar(0);
    TestErrorCode("glcRenderChar");

    glcRenderCountedString(0, NULL);
    TestErrorCode("glcRenderCountedString");

    glcRenderString(NULL);
    TestErrorCode("glcRenderString");

    glcRenderStyle(GLC_LINE);
    TestErrorCode("glcRenderStyle");

    glcReplacementCode(0);
    TestErrorCode("glcReplacementCode");

    glcResolution(0.);
    TestErrorCode("glcResolution");

    /* Measurement commands*/

    glcGetCharMetric(0, GLC_BOUNDS, NULL);
    TestErrorCode("glcGetCharMetric");

    glcGetMaxCharMetric(GLC_BASELINE, NULL);
    TestErrorCode("glcGetMaxCharMetric");

    glcGetStringCharMetric(0, GLC_BOUNDS, NULL);
    TestErrorCode("glcGetStringCharMetric");

    glcGetStringMetric(GLC_BOUNDS, 0);
    TestErrorCode("glcGetStringMetric");

    glcMeasureCountedString(GL_TRUE, 1, NULL);
    TestErrorCode("glcMeasureCountedString");

    glcMeasureString(GL_FALSE, NULL);
    TestErrorCode("glcMeasureString");

    printf("Tests successful !\n");
    return 0;
}
