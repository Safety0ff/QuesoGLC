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
#include "unichar.h"

#define GLC_STRING_CHUNK	256

class __glcStringList {
  __glcUniChar* string;	/* Pointer to the string */
  __glcStringList* next;
  GLuint count;

 public:
  inline GLint getCount(void) { return count; }

  __glcStringList(__glcUniChar* inString);
  ~__glcStringList();
  GLint append(__glcUniChar* inString);
  GLint prepend(__glcUniChar* inString);
  GLint remove(__glcUniChar* inString);
  GLint removeIndex(GLuint inIndex);
  __glcUniChar* find(__glcUniChar* inString);
  __glcUniChar* findIndex(GLuint inIndex);
  GLint getIndex(__glcUniChar* inString);
  GLint convert(int inType);
};

#endif /* __glc_strlst_h */
