/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2006, Bertrand Coconnier
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

#ifndef __glc_ocharmap_h
#define __glc_ocharmap_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#include "GL/glc.h"
#include "ocontext.h"
#include "ofacedesc.h"

typedef struct {
  __glcFaceDescriptor* faceDesc;
  FcCharSet* charSet;
  FT_ULong (*map)[3];
  GLint count;
  GLint length;
} __glcCharMap;

__glcCharMap* __glcCharMapCreate(__glcFaceDescriptor* inFaceDesc);
void __glcCharMapDestroy(__glcCharMap* This);
void __glcCharMapAddChar(__glcCharMap* This, GLint inCode,
		    const GLCchar* inCharName, __glcContextState* inState);
void __glcCharMapRemoveChar(__glcCharMap* This, GLint inCode);
GLCchar* __glcCharMapGetCharName(__glcCharMap* This, GLint inCode,
				 __glcContextState* inState);
FT_UInt __glcCharMapGlyphIndex(__glcCharMap* This, FT_Face inFace,
			       GLint inCode);
GLboolean __glcCharMapHasChar(__glcCharMap* This, GLint inCode);
GLCchar* __glcCharMapGetCharNameByIndex(__glcCharMap* This, GLint inIndex,
					__glcContextState* inState);
GLint __glcCharMapGetCount(__glcCharMap* This);
GLint __glcCharMapGetMaxMappedCode(__glcCharMap* This);
GLint __glcCharMapGetMinMappedCode(__glcCharMap* This);
#endif
