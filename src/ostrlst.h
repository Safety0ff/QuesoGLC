/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
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
#ifndef __glc_strlst_h
#define __glc_strlst_h

#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "ounichar.h"

#define GLC_STRING_CHUNK	256

typedef struct __glcStringList__ {
  __glcUniChar* string;	/* Pointer to the string */
  struct __glcStringList__* next;
  GLuint count;
} __glcStringList;

__glcStringList* __glcStrLstCreate(__glcUniChar* inString);
void __glcStrLstDestroy(__glcStringList* This);
GLint __glcStrLstAppend(__glcStringList* This, __glcUniChar* inString);
GLint __glcStrLstPrepend(__glcStringList* This, __glcUniChar* inString);
GLint __glcStrLstRemove(__glcStringList* This, __glcUniChar* inString);
GLint __glcStrLstRemoveIndex(__glcStringList* This, GLuint inIndex);
__glcUniChar* __glcStrLstFind(__glcStringList* This, __glcUniChar* inString);
__glcUniChar* __glcStrLstFindIndex(__glcStringList* This, GLuint inIndex);
GLint __glcStrLstGetIndex(__glcStringList* This, __glcUniChar* inString);
GLint __glcStrLstConvert(__glcStringList* This, int inType);

#endif /* __glc_strlst_h */
