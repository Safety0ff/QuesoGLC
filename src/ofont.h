/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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

/** \file
 * header of the object __GLCfont which manage the fonts
 */

#ifndef __glc_ofont_h
#define __glc_ofont_h

#include "ofacedesc.h"

/* It seems that Visual C++ does not recognize the inline keyword !?! */
#ifdef _MSC_VER
#define inline
#endif

struct __GLCfontRec {
  GLint id;
  __GLCfaceDescriptor* faceDesc;
  GLint parentMasterID;
  __GLCcharMap* charMap;
};

__GLCfont*  __glcFontCreate(GLint id, __GLCmaster* inMaster,
			    __GLCcontext* inContext, GLint inCode);
void __glcFontDestroy(__GLCfont *This, __GLCcontext* inContext);
__GLCglyph* __glcFontGetGlyph(__GLCfont *This, GLint inCode,
			      __GLCcontext* inContext);
GLfloat* __glcFontGetBoundingBox(__GLCfont *This, GLint inCode,
				 GLfloat* outVec, __GLCcontext* inContext,
				 GLfloat inScaleX, GLfloat inScaleY);
GLfloat* __glcFontGetAdvance(__GLCfont *This, GLint inCode, GLfloat* outVec,
			     __GLCcontext* inContext, GLfloat inScaleX,
			     GLfloat inScaleY);
GLfloat* __glcFontGetKerning(__GLCfont* This, GLint inCode, GLint inPrevCode,
			     GLfloat* outVec, __GLCcontext* inContext,
			     GLfloat inScaleX, GLfloat inScaleY);
#ifndef GLC_FT_CACHE
inline void* __glcFontOpen(__GLCfont* This, __GLCcontext* inContext);
inline void __glcFontClose(__GLCfont* This);
#endif
GLboolean __glcFontPrepareGlyph(__GLCfont* This, __GLCcontext* inContext,
				GLfloat inScaleX, GLfloat inScaleY,
				GLCulong inGlyphIndex);
inline GLboolean __glcFontGetBitmapSize(__GLCfont* This, GLint* outWidth,
					GLint *outHeight, GLint* outBoundingBox,
					GLfloat inScaleX, GLfloat inScaleY,
					int inFactor, __GLCcontext* inContext);
inline GLfloat* __glcFontGetMaxMetric(__GLCfont* This, GLfloat* outVec,
				      __GLCcontext* inContext);
inline GLboolean __glcFontOutlineDecompose(__GLCfont* This,
					   __GLCrendererData* inData,
					   __GLCcontext* inContext);
inline GLboolean __glcFontGetBitmap(__GLCfont* This, GLint inWidth,
				    GLint inHeight, void* inBuffer,
				    __GLCcontext* inContext);
inline GLboolean __glcFontOutlineEmpty(__GLCfont* This);
#endif /* __glc_ofont_h */
