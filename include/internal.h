/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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
#if !defined __APPLE__ && !defined __MACH__
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

#define GLC_OUT_OF_RANGE_LEN 11

  typedef struct {
    __glcFaceDescriptor* faceDesc;
    GLint code;
    GLint renderMode;
    GLuint list;
  } __glcDisplayListKey;

  typedef struct {
    GLint code;
    GLCchar* name;
  } __glcDataCodeFromName;

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
  extern void __glcDeleteGLObjects(__glcContextState *inState);

  extern const __glcDataCodeFromName __glcCodeFromNameArray[];
  extern const GLint __glcNameFromCodeArray[];
  extern const GLint __glcMaxCode;
  extern const GLint __glcCodeFromNameSize;
  /* Find a Unicode name from its code */
  extern GLCchar* __glcNameFromCode(GLint code);
  /* Find a Unicode code from its name */
  extern GLint __glcCodeFromName(GLCchar* name);
  /* Create an initialize a FreeType  double linked list */
  extern GLboolean __glcCreateList(FT_List* list);

  /* Duplicate a string in UTF-8 format */
  extern FcChar8* __glcConvertToUtf8(const GLCchar* inString,
				     const GLint inStringType);
  /* Duplicate a UTF-8 string to another format */
  extern GLCchar* __glcConvertFromUtf8(const FcChar8* inString,
				       const GLint inStringType);

  /* Duplicate a UTF-8 string to the context buffer */
  extern GLCchar* __glcConvertFromUtf8ToBuffer(__glcContextState* This,
					       const FcChar8* inString,
					       const GLint inStringType);
  /* Count the number of bits that are set in c1  */
  extern FcChar32 FcCharSetPopCount(FcChar32 c1);
#ifdef __cplusplus
}
#endif

#endif /* __glc_internal_h */
