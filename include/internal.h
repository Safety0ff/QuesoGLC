/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */

#ifndef __glc_internal_h
#define __glc_internal_h

#include <stdlib.h>

#include "GL/glc.h"
#ifndef __MACOSX__
#include "qglc_config.h"
#endif
#include "ofont.h"
#include "ocontext.h"

#ifndef DEBUGMODE
#define NDEBUG 1
#endif

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
