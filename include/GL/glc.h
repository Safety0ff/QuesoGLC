#if !defined(__glc_h_)
#define __glc_h_

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#if defined(__cplusplus)
    extern "C" {
#endif

typedef void GLCchar;
typedef GLint GLCenum;

#if defined(__cplusplus)
typedef GLboolean (*GLCfunc)(...);
#else
typedef GLboolean (*GLCfunc)();
#endif

/*************************************************************/

#define GLC_NONE                                  0x0000

#define GLC_AUTO_FONT                             0x0010
#define GLC_GL_OBJECTS                            0x0011
#define GLC_MIPMAP                                0x0012

#define GLC_OP_glcUnmappedCode                    0x0020

#define GLC_BASELINE                              0x0030
#define GLC_BOUNDS                                0x0031

#define GLC_PARAMETER_ERROR                       0x0040
#define GLC_RESOURCE_ERROR                        0x0041
#define GLC_STATE_ERROR                           0x0042

#define GLC_CHAR_LIST                             0x0050
#define GLC_FACE_LIST                             0x0051

#define GLC_FAMILY                                0x0060
#define GLC_MASTER_FORMAT                         0x0061
#define GLC_VENDOR                                0x0062
#define GLC_VERSION                               0x0063

#define GLC_CHAR_COUNT                            0x0070
#define GLC_FACE_COUNT                            0x0071
#define GLC_IS_FIXED_PITCH                        0x0072
#define GLC_MAX_MAPPED_CODE                       0x0073
#define GLC_MIN_MAPPED_CODE                       0x0074
#define GLC_IS_OUTLINE                            0x0075

#define GLC_CATALOG_LIST                          0x0080

#define GLC_CURRENT_FONT_LIST                     0x0090
#define GLC_FONT_LIST                             0x0091
#define GLC_LIST_OBJECT_LIST                      0x0092
#define GLC_TEXTURE_OBJECT_LIST                   0x0093

#define GLC_DATA_POINTER                          0x00A0

#define GLC_EXTENSIONS                            0x00B0
#define GLC_RELEASE                               0x00B1

#define GLC_RESOLUTION                            0x00C0

#define GLC_BITMAP_MATRIX                         0x00D0

#define GLC_CATALOG_COUNT                         0x00E0
#define GLC_CURRENT_FONT_COUNT                    0x00E1
#define GLC_FONT_COUNT                            0x00E2
#define GLC_LIST_OBJECT_COUNT                     0x00E3
#define GLC_MASTER_COUNT                          0x00E4
#define GLC_MEASURED_CHAR_COUNT                   0x00E5
#define GLC_RENDER_STYLE                          0x00E6
#define GLC_REPLACEMENT_CODE                      0x00E7
#define GLC_STRING_TYPE                           0x00E8
#define GLC_TEXTURE_OBJECT_COUNT                  0x00E9
#define GLC_VERSION_MAJOR                         0x00EA
#define GLC_VERSION_MINOR                         0x00EB

#define GLC_BITMAP                                0x0100
#define GLC_LINE                                  0x0101
#define GLC_TEXTURE                               0x0102
#define GLC_TRIANGLE                              0x0103

#define GLC_UCS1                                  0x0110
#define GLC_UCS2                                  0x0111
#define GLC_UCS4                                  0x0112
#define GLC_UTF8                                  0x0113

/*************************************************************/

extern void glcContext (GLint inContext);
extern void glcDeleteContext (GLint inContext);
extern GLint glcGenContext (void);
extern GLint* glcGetAllContexts (void);
extern GLint glcGetCurrentContext (void);
extern GLCenum glcGetError (void);
extern GLboolean glcIsContext (GLint inContext);

extern void glcCallbackFunc (GLCenum inOpcode, GLCfunc inFunc);
extern void glcDataPointer (GLvoid *inPointer);
extern void glcDeleteGLObjects (void);
extern void glcDisable (GLCenum inAttrib);
extern void glcEnable (GLCenum inAttrib);
extern GLCfunc glcGetCallbackFunc (GLCenum inOpcode);
extern const GLCchar* glcGetListc (GLCenum inAttrib, GLint inIndex);
extern GLint glcGetListi (GLCenum inAttrib, GLint inIndex);
extern GLvoid* glcGetPointer (GLCenum inAttrib);
extern const GLCchar* glcGetc (GLCenum inAttrib);
extern GLfloat glcGetf (GLCenum inAttrib);
extern GLfloat* glcGetfv (GLCenum inAttrib, GLfloat *outVec);
extern GLint glcGeti (GLCenum inAttrib);
extern GLboolean glcIsEnabled (GLCenum inAttrib);
extern void glcStringType (GLCenum inStringType);

extern void glcAppendCatalog (const GLCchar *inCatalog);
extern const GLCchar* glcGetMasterListc (GLint inMaster, GLCenum inAttrib, GLint inIndex);
extern GLint glcGetMasterListiSGI (GLint inMaster, GLCenum inAttrib, GLint inIndex);
extern const GLCchar* glcGetMasterMap (GLint inMaster, GLint inCode);
extern const GLCchar* glcGetMasterc (GLint inMaster, GLCenum inAttrib);
extern GLint glcGetMasteri (GLint inMaster, GLCenum inAttrib);
extern void glcPrependCatalog (const GLCchar *inCatalog);
extern void glcRemoveCatalog (GLint inIndex);

extern void glcAppendFont (GLint inFont);
extern void glcDeleteFont (GLint inFont);
extern void glcFont (GLint inFont);
extern GLboolean glcFontFace (GLint inFont, const GLCchar *inFace);
extern void glcFontMap (GLint inFont, GLint inCode, const GLCchar *inCharName);
extern GLint glcGenFontID (void);
extern const GLCchar* glcGetFontFace (GLint inFont);
extern const GLCchar* glcGetFontListc (GLint inFont, GLCenum inAttrib, GLint inIndex);
extern GLint glcGetFontListiSGI (GLint inFont, GLCenum inAttrib, GLint inIndex);
extern const GLCchar* glcGetFontMap (GLint inFont, GLint inCode);
extern const GLbyte* glcGetFontMasterArray (GLint inFont, GLboolean inFull, GLint *outCount);
extern const GLCchar* glcGetFontc (GLint inFont, GLCenum inAttrib);
extern GLint glcGetFonti (GLint inFont, GLCenum inAttrib);
extern GLboolean glcIsFont (GLint inFont);
extern GLint glcNewFontFromFamily (GLint inFont, const GLCchar *inFamily);
extern GLint glcNewFontFromMaster (GLint inFont, GLint inMaster);

extern void glcLoadIdentity (void);
extern void glcLoadMatrix (const GLfloat *inMatrix);
extern void glcMultMatrix (const GLfloat *inMatrix);
extern void glcRotate (GLfloat inAngle);
extern void glcScale (GLfloat inX, GLfloat inY);

extern void glcRenderChar (GLint inCode);
extern void glcRenderCountedString (GLint inCount, const GLCchar *inString);
extern void glcRenderString (const GLCchar *inString);
extern void glcRenderStyle (GLCenum inStyle);
extern void glcReplacementCode (GLint inCode);
extern void glcResolution (GLfloat inVal);

extern GLfloat* glcGetCharMetric (GLint inCode, GLCenum inMetric, GLfloat *outVec);
extern GLfloat* glcGetMaxCharMetric (GLCenum inMetric, GLfloat *outVec);
extern GLfloat* glcGetStringCharMetric (GLint inIndex, GLCenum inMetric, GLfloat *outVec);
extern GLfloat* glcGetStringMetric (GLCenum inMetric, GLfloat *outVec);
extern GLint glcMeasureCountedString (GLboolean inMeasureChars, GLint inCount, const GLCchar *inString);
extern GLint glcMeasureString (GLboolean inMeasureChars, const GLCchar *inString);

#if defined (__cplusplus)
    }
#endif

#endif /* defined (__glc_h_) */
