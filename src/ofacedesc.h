/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
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

#ifndef __glc_ofacedesc_h
#define __glc_ofacedesc_h

typedef struct {
  FT_ListNodeRec node;
  FcChar8* styleName;
  GLboolean isFixedPitch;	/* GLC_IS_FIXED_PITCH */
  FcChar8* fileName;
  FT_Long indexInFile;
#ifndef FT_CACHE_H
  FT_Face face;
  int faceRefCount;
#endif
  FT_ListRec glyphList;
} __glcFaceDescriptor;


__glcFaceDescriptor* __glcFaceDescCreate(FcChar8* inStyleName,
					 FcCharSet* inCharSet,
					 GLboolean inIsFixedPitch,
					 FcChar8* inFileName,
					 FT_Long inIndexInFile);
void __glcFaceDescDestroy(__glcFaceDescriptor* This,
			  __glcContextState* inState);
#ifndef FT_CACHE_H
FT_Face __glcFaceDescOpen(__glcFaceDescriptor* This,
			  __glcContextState* inState);
void __glcFaceDescClose(__glcFaceDescriptor* This);
#endif
__glcGlyph* __glcFaceDescGetGlyph(__glcFaceDescriptor* This, GLint inCode,
				  __glcContextState* inState);
FT_Face __glcFaceDescLoadFreeTypeGlyph(__glcFaceDescriptor* This,
			       __glcContextState* inState, GLfloat inScaleX,
			       GLfloat inScaleY, FT_ULong inGlyphIndex);
void __glcFaceDescDestroyGLObjects(__glcFaceDescriptor* This);
__glcCharMap* __glcFaceDescGetCharMap(__glcFaceDescriptor* This,
				      __glcContextState* inState);
GLfloat* __glcFaceDescGetBoundingBox(__glcFaceDescriptor* This,
				     FT_ULong inGlyphIndex, GLfloat* outVec,
				     GLfloat inScaleX, GLfloat inScaleY,
				     __glcContextState* inState);
GLfloat* __glcFaceDescGetAdvance(__glcFaceDescriptor* This,
				 FT_ULong inGlyphIndex, GLfloat* outVec,
				 GLfloat inScaleX, GLfloat inScaleY,
				 __glcContextState* inState);
FcChar8* __glcFaceDescGetFontFormat(__glcFaceDescriptor* This,
				    __glcContextState* inState,
				    GLCenum inAttrib);
GLfloat* __glcFaceDescGetMaxMetric(__glcFaceDescriptor* This, GLfloat* outVec,
				   __glcContextState* inState);
GLfloat* __glcFaceDescGetKerning(__glcFaceDescriptor* This,
				 FT_UInt inGlyphIndex, FT_UInt inPrevGlyphIndex,
				 GLfloat inScaleX, GLfloat inScaleY,
				 GLfloat* outVec, __glcContextState* inState);
#endif /* __glc_ofacedesc_h */
