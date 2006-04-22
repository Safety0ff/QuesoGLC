/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2006, Bertrand Coconnier
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

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "internal.h"

__glcFaceDescriptor* __glcFaceDescCreate(FcChar8* inStyleName,
					 FcCharSet* inCharSet,
					 GLboolean inIsFixedPitch,
					 FcChar8* inFileName,
					 FT_Long inIndexInFile)
{
  __glcFaceDescriptor* This = NULL;

  This = (__glcFaceDescriptor*)__glcMalloc(sizeof(__glcFaceDescriptor));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->styleName = (FcChar8*)strdup((const char*)inStyleName);
  if (!This->styleName) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->charSet = FcCharSetCopy(inCharSet);
  if (!This->charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(This->styleName);
    __glcFree(This);
    return NULL;
  }

  /* Filenames are kept in their original format which is supposed to
   * be compatible with strlen()
   */
  This->fileName = (FcChar8*)strdup((const char*)inFileName);
  if (!This->fileName) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcCharSetDestroy(This->charSet);
    free(This->styleName);
    __glcFree(This);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;
  This->isFixedPitch = inIsFixedPitch;
  This->indexInFile = inIndexInFile;
  This->face = NULL;
  This->faceRefCount = 0;

  return This;
}



void __glcFaceDescDestroy(__glcFaceDescriptor* This)
{
  free(This->styleName);
  free(This->fileName);
  FcCharSetDestroy(This->charSet);
  __glcFree(This);
}



FT_Face __glcFaceDescOpen(__glcFaceDescriptor* This,
			  __glcContextState* inState)
{
  if (!This->faceRefCount) {
    if (FT_New_Face(inState->library, (const char*)This->fileName,
		    This->indexInFile, &This->face)) {
      /* Unable to load the face file */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    /* select a Unicode charmap */
    if (FT_Select_Charmap(This->face, ft_encoding_unicode)) {
      /* No Unicode charmap is available */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FT_Done_Face(This->face);
      This->face = NULL;
      return NULL;
    }

    This->faceRefCount = 1;
  }
  else
    This->faceRefCount++;

  return This->face;
}



void __glcFaceDescClose(__glcFaceDescriptor* This)
{
  assert(This->faceRefCount > 0);

  This->faceRefCount--;

  if (!This->faceRefCount) {
    assert(This->face);

    FT_Done_Face(This->face);
    This->face = NULL;
  }
}



FT_UInt __glcFaceDescGetGlyphIndex(__glcFaceDescriptor* This, GLint inCode,
				   __glcContextState* inState)
{
  FT_Face face = __glcFaceDescOpen(This, inState);
  FT_UInt glyph = 0;

  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0xffffffff;
  }

  /* Verify that the character exists in the face */
  if (!FcCharSetHasChar(This->charSet, inCode)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0xffffffff;
  }

  glyph = FcFreeTypeCharIndex(face, inCode);

  __glcFaceDescClose(This);
  return glyph;
}



FT_Face __glcFaceDescLoadGlyph(__glcFaceDescriptor* This,
			       __glcContextState* inState, GLfloat inScaleX,
			       GLfloat inScaleY, FT_ULong inGlyphIndex)
{
  FT_Face face = NULL;
  FT_Int32 loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM;

  if (!inState->hinting)
    loadFlags |= FT_LOAD_NO_HINTING;

  face = __glcFaceDescOpen(This, inState);
  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (FT_Set_Char_Size(face, (FT_F26Dot6)(inScaleX * 64.),
		       (FT_F26Dot6)(inScaleY * 64.),
		       (FT_UInt)inState->resolution,
		       (FT_UInt)inState->resolution)) {
    __glcFaceDescClose(This);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (FT_Load_Glyph(face, inGlyphIndex, loadFlags)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(This);
    return NULL;
  }

  return face;
}
