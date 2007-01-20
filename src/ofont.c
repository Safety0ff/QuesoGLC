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

/* Defines the methods of an object that is intended to managed fonts */

#include "internal.h"

/** \file
 * defines the object __glcFont which manage the fonts
 */

/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the master 'inParent' which the font will instantiate.
 */
__glcFont* __glcFontCreate(GLint inID, GLint inMaster,
			   __glcContextState* inState)
{
  __glcFont *This = NULL;
  FcPattern* pattern = __glcGetPatternFromMasterID(inMaster, inState);

  assert(inState);

  if (!pattern)
    return NULL;

  This = (__glcFont*)__glcMalloc(sizeof(__glcFont));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  /* At font creation, the default face is the first one.
   * glcFontFace() can change the face.
   */
  This->faceDesc = __glcGetFaceDescFromPattern(pattern);
  FcPatternDestroy(pattern);
  if (!This->faceDesc) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->charMap = __glcFaceDescGetCharMap(This->faceDesc, inState);
  if (!This->charMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescDestroy(This->faceDesc, inState);
    __glcFree(This);
    return NULL;
  }

  This->parentMasterID = inMaster;
  This->id = inID;

  return This;
}



/* Destructor of the object */
void __glcFontDestroy(__glcFont *This, __glcContextState* inState)
{
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  if (This->faceDesc)
    __glcFaceDescDestroy(This->faceDesc, inState);

  __glcFree(This);
}



/* Extract from the font the glyph which corresponds to the character code
 * 'inCode'.
 */
__glcGlyph* __glcFontGetGlyph(__glcFont *This, GLint inCode,
			      __glcContextState* inState)
{
  /* Try to get the glyph from the character map */
  __glcGlyph* glyph = __glcCharMapGetGlyph(This->charMap, inCode);

  if (!glyph) {
    /* If it fails, we must extract the glyph from the face */
    glyph = __glcFaceDescGetGlyph(This->faceDesc, inCode, inState);
    if (!glyph) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return NULL;
    }
    /* Update the character map so that the glyph will be cached */
    __glcCharMapAddChar(This->charMap, inCode, glyph);
  }

  return glyph;
}



/* Get the bounding box of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inCode' contains the character
 * code for which the bounding box is requested.
 */
GLfloat* __glcFontGetBoundingBox(__glcFont *This, GLint inCode,
				 GLfloat* outVec, __glcContextState* inState,
				 GLfloat inScaleX, GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __glcGlyph* glyph = __glcFontGetGlyph(This, inCode, inState);

  assert(outVec);

  if (!glyph)
    return NULL;

  /* If the bounding box of the glyph is cached then copy it to outVec and
   * return.
   */
  if (glyph->boundingBox[0] || glyph->boundingBox[1] || glyph->boundingBox[2]
      || glyph->boundingBox[3]) {
    memcpy(outVec, glyph->boundingBox, 4 * sizeof(GLfloat));
    return outVec;
  }

  /* Otherwise, we must extract the bounding box from the face file */
  if (!__glcFaceDescGetBoundingBox(This->faceDesc, glyph->index,
				   glyph->boundingBox, inScaleX, inScaleY,
				   inState)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Copy the result to outVec and return */
  memcpy(outVec, glyph->boundingBox, 4 * sizeof(GLfloat));
  return outVec;
}



/* Get the advance of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inCode' contains the character
 * code for which the advance is requested.
 */
GLfloat* __glcFontGetAdvance(__glcFont* This, GLint inCode, GLfloat* outVec,
			     __glcContextState* inState, GLfloat inScaleX,
			     GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __glcGlyph* glyph = __glcFontGetGlyph(This, inCode, inState);

  assert(outVec);

  if (!glyph)
    return NULL;

  /* If the advance of the glyph is cached then copy it to outVec and
   * return.
   */
  if (glyph->advance[0] || glyph->advance[1]) {
    memcpy(outVec, glyph->advance, 2 * sizeof(GLfloat));
    return outVec;
  }

  /* Otherwise, we must extract the advance from the face file */
  if (!__glcFaceDescGetAdvance(This->faceDesc, glyph->index, glyph->advance,
			       inScaleX, inScaleY, inState)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Copy the result to outVec and return */
  memcpy(outVec, glyph->advance, 2 * sizeof(GLfloat));
  return outVec;
}



/* Get the kerning information of a pair of glyph according to the size given by
 * inScaleX and inScaleY. The result is returned in outVec. 'inCode' contains
 * the current character code and 'inPrevCode' the character code of the
 * previously displayed character.
 */
GLfloat* __glcFontGetKerning(__glcFont* This, GLint inCode, GLint inPrevCode,
			     GLfloat* outVec, __glcContextState* inState,
			     GLfloat inScaleX, GLfloat inScaleY)
{
  __glcGlyph* glyph = __glcFontGetGlyph(This, inCode, inState);
  __glcGlyph* prevGlyph = __glcFontGetGlyph(This, inPrevCode, inState);

  if (!glyph || !prevGlyph)
    return NULL;

  return __glcFaceDescGetKerning(This->faceDesc, glyph->index, prevGlyph->index,
				 inScaleX, inScaleY, outVec, inState);
}
