#ifndef __glc_ocontext_h
#define __glc_ocontext_h

#include <pthread.h>
#include <gdbm.h>

#include "GL/glc.h"
#include "constant.h"
#include "ofont.h"
#include "omaster.h"
#include "strlst.h"

extern "C" {
  void my_fini(void);
}

class __glcContextState {
  static GLboolean *isCurrent;
  static __glcContextState **stateList;
  static pthread_once_t initOnce;
  static pthread_mutex_t mutex;
  static pthread_key_t contextKey;
  static pthread_key_t errorKey;

  static void initQueso(void);
  static __glcContextState* getState(GLint inContext);
  static void setState(GLint inContext, __glcContextState *inState);
  static GLboolean getCurrency(GLint inContext);
  static void setCurrency(GLint inContext, GLboolean isCurrent);
  static GLboolean isContext(GLint inContext);

 public:
  static FT_Library library;
  static GDBM_FILE unidb1, unidb2;

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
  char __glcBuffer[GLC_STRING_CHUNK];

  __glcContextState(GLint inContext);
  ~__glcContextState();

  static __glcContextState* getCurrent(void);
  static void raiseError(GLCenum inError);

  friend void glcContext(GLint inContext);
  friend void glcDeleteContext(GLint inContext);
  friend GLint glcGenContext(void);
  friend GLint* glcGetAllContexts(void);
  friend GLint glcGetCurrentContext(void);
  friend GLCenum glcGetError(void);
  friend GLboolean glcIsContext(GLint inContext);

  friend void my_fini(void);
};

#endif /* __glc_ocontext_h */
