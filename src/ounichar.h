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

#ifndef __glc_unichar_h
#define __glc_unichar_h

#include "GL/glc.h"
#include <string.h>

typedef struct {
  GLCchar *ptr;
  GLint type;
} __glcUniChar;

size_t __glcUniLen(__glcUniChar *This);
size_t __glcUniLenBytes(__glcUniChar *This);
GLCchar* __glcUniDup(__glcUniChar *This, GLCchar* dest, size_t n);
int __glcUniCompare(__glcUniChar *This, __glcUniChar* inString);
GLCchar* __glcUniConvert(__glcUniChar *This, GLCchar* dest, int inType,
			 size_t n);
int __glcUniEstimate(__glcUniChar *This, int inType);
void __glcUniDestroy(__glcUniChar *This);
GLuint __glcUniIndex(__glcUniChar *This, int inPos);

#endif
