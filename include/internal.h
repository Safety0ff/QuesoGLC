/* $Id$ */
#include "GL/glc.h"
#include "ofont.h"
#include "ocontext.h"

#ifndef __glc_internal_h
#define __glc_internal_h

#ifdef __cplusplus
extern "C" {
#endif

extern char* strdup(const char *s);

typedef struct {
  GLint face;
  GLint code;
  GLint renderMode;
} __glcDisplayListKey;

/* Character renderers */
extern void __glcRenderCharScalable(__glcFont* inFont, __glcContextState* inState, GLint inCode, destroyFunc destroyKey, destroyFunc destroyData, compareFunc compare, GLboolean inFill);

#ifdef __cplusplus
}
#endif

#endif /* __glc_internal_h */
