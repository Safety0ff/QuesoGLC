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
#ifndef __glc_ofont_h
#define __glc_ofont_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glc.h"
#include "constant.h"
#include "omaster.h"

typedef struct {
  GLint id;
  GLint faceID;
  __glcMaster *parent;
  FT_Face face;
  FT_ULong charMap[2][GLC_MAX_CHARMAP];
  GLint charMapCount;
} __glcFont;

__glcFont*  __glcFontCreate(GLint id, __glcMaster *parent);
void __glcFontDestroy(__glcFont *This);

#endif /* __glc_ofont_h */
