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
#ifndef __glc_unichar_h
#define __glc_unichar_h

#include "GL/glc.h"
#include <string.h>

typedef union {
  unsigned char *ucs1;
  unsigned short *ucs2;
  unsigned int *ucs4;
} uniChar;

class __glcStringList;

class __glcUniChar {
  GLCchar *ptr;
  GLint type;

 public:
  friend __glcStringList;

  __glcUniChar();
  __glcUniChar(const GLCchar* inChar, GLint inType = GLC_UCS1);
  ~__glcUniChar() {}

  size_t len(void);
  size_t lenBytes(void);
  GLCchar* dup(GLCchar* dest, size_t n)
    { return memmove(dest, ptr, lenBytes() > n ? n : lenBytes()); }
  int compare(__glcUniChar* inString);
  GLCchar* convert(GLCchar* dest, int inType,size_t n);
  int estimate(int inType);
  void destroy(void);
  GLuint index(int inPos);
};

#endif
