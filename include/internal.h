/* $Id$ */
#include <stdlib.h>

#include "GL/glc.h"
#include "qglc_config.h"
#include "ofont.h"
#include "ocontext.h"

#ifndef __glc_internal_h
#define __glc_internal_h

#ifdef __cplusplus
extern "C" {
#endif

#ifdef QUESOGLC_GCC3
  extern char* strdup(const char *s);
#endif

  typedef struct {
    GLint face;
    GLint code;
    GLint renderMode;
    GLuint list;
  } __glcDisplayListKey;

  /* Character renderers */
  extern void __glcRenderCharScalable(__glcFont* inFont,
				      __glcContextState* inState, GLint inCode,
				      GLboolean inFill);

  /* QuesoGLC own memory management routines */
  extern void* __glcMalloc(size_t size);
  extern void __glcFree(void* ptr);
  extern void* __glcRealloc(void* ptr, size_t size);

  /* FT double linked list destructor */
  extern void __glcListDestructor(FT_Memory inMemory, void *inData,
				  void *inUser);

  /* Find a token in a list of tokens separated by 'separator' */
  extern GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex,
				     const GLCchar* inSeparator);

  /* Deletes GL objects defined in context state */
  void __glcDeleteGLObjects(__glcContextState *inState);

  /* Fetch a key in the DB */
  datum __glcDBFetch(DBM *dbf, datum key);

#ifdef __cplusplus
}
#endif

#endif /* __glc_internal_h */
