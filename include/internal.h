/* $Id$ */
#include <ft2build.h>
#include FT_FREETYPE_H
#include <gdbm.h>

#include "GL/glc.h"
#include "strlst.h"

#define GLC_INTERNAL_ERROR 	0x0043

#define GLC_MAX_CONTEXTS	16
#define GLC_MAX_CURRENT_FONT	16
#define GLC_MAX_FONT		16
#define GLC_MAX_LIST_OBJECT	16
#define GLC_MAX_TEXTURE_OBJECT	16
#define GLC_MAX_MASTER		16
#define GLC_MAX_FACE_PER_MASTER	16
#define GLC_MAX_MEASURE		GLC_STRING_CHUNK
#define GLC_MAX_CHARMAP		256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GLint id;
    __glcStringList* faceList;	/* GLC_FACE_LIST */
    GLint charListCount;	/* GLC_CHAR_LIST_COUNT */
    GLCchar* family;		/* GLC_FAMILY */
    GLCchar* masterFormat;	/* GLC_MASTER_FORMAT */
    GLCchar* vendor;		/* GLC_VENDOR */
    GLCchar* version;		/* GLC_VERSION */
    GLint isFixedPitch;		/* GLC_IS_FIXED_PITCH */
    GLint maxMappedCode;	/* GLC_MAX_MAPPED_CODE */
    GLint minMappedCode;	/* GLC_MIN_MAPPED_CODE */
    __glcStringList* faceFileName;
} __glcMaster;

typedef struct {
    GLint id;
    GLint faceID;
    __glcMaster *parent;
    FT_Face face;
    GLint charMap[2][GLC_MAX_CHARMAP];
    GLint charMapCount;
} __glcFont;

typedef struct {
    GLint id;			/* Context ID */
    GLboolean pendingDelete;	/* Is there a pending deletion ? */
    GLCfunc callback;		/* Callback function */
    GLvoid* dataPointer;	/* GLC_DATA_POINTER */
    GLboolean autoFont;		/* GLC_AUTO_FONT */
    GLboolean glObjects;	/* GLC_GLOBJECTS */
    GLboolean mipmap;		/* GLC_MIPMAP */
    __glcStringList* catalogList;/* GLC_CATALOG_LIST */
    GLfloat resolution;		/* GLC_RESOLUTION */
    GLfloat bitmapMatrix[4];	/* GLC_BITMAP_MATRIX */
    GLint currentFontList[GLC_MAX_CURRENT_FONT];
    GLint currentFontCount;	/* GLC_CURRENT_FONT_COUNT */
    __glcFont* fontList[GLC_MAX_FONT];
    GLint fontCount;		/* GLC_FONT_COUNT */
    GLuint listObjectList[GLC_MAX_LIST_OBJECT];
    GLint listObjectCount;	/* GLC_LIST_OBJECT_COUNT */
    __glcMaster* masterList[GLC_MAX_MASTER];
    GLint masterCount;		/* GLC_MASTER_COUNT */
    GLint measuredCharCount;	/* GLC_MEASURED_CHAR_COUNT */
    GLint renderStyle;		/* GLC_RENDER_STYLE */
    GLint replacementCode;	/* GLC_REPLACEMENT_CODE */
    GLint stringType;		/* GLC_STRING_TYPE */
    GLuint textureObjectList[GLC_MAX_TEXTURE_OBJECT];
    GLint textureObjectCount;	/* GLC_TEXTURE_OBJECT_COUNT */
    GLint versionMajor;		/* GLC_VERSION_MAJOR */
    GLint versionMinor;		/* GLC_VERSION_MINOR */
    GLfloat measurementCharBuffer[GLC_MAX_MEASURE][12];
    GLfloat measurementStringBuffer[12];
    GLboolean isInCallbackFunc;
} __glcContextState;

/* Global internal variables */
extern FT_Library library;
extern char __glcBuffer[GLC_STRING_CHUNK];
extern GDBM_FILE unicod1;
extern GDBM_FILE unicod2;

/* Global internal commands */
extern void __glcRaiseError(GLCenum inError);
extern __glcContextState* __glcGetCurrentState(void);

/* Font and rendering internal commands */
extern GLint __glcGetFont(GLint inCode);

/* Character renderers */
extern void __glcRenderCharScalable(__glcFont* inFont, __glcContextState* inState, GLboolean inFill);

/* Master helpers */
extern void __glcDeleteMaster(GLint inMaster, __glcContextState *inState);

/* Temporary functions */
extern void my_init(void);
extern void my_fini(void);

#ifdef __cplusplus
}
#endif
