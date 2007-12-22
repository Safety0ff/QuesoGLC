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

/** \file
 * defines the object __GLCfont which manage the fonts
 */

#include "internal.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the master 'inParent' which the font will instantiate.
 */
__GLCfont* __glcFontCreate(GLint inID, __GLCmaster* inMaster,
			   __GLCcontext* inContext)
{
  __GLCfont *This = NULL;

  assert(inContext);
  assert(inMaster);

  This = (__GLCfont*)__glcMalloc(sizeof(__GLCfont));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  /* At font creation, the default face is the first one.
   * glcFontFace() can change the face.
   */
  This->faceDesc = __glcFaceDescCreate(inMaster, NULL, inContext);
  if (!This->faceDesc) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->charMap = __glcCharMapCreate(inMaster, inContext);
  if (!This->charMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescDestroy(This->faceDesc, inContext);
    __glcFree(This);
    return NULL;
  }

  This->parentMasterID = __glcMasterGetID(inMaster, inContext);
  This->id = inID;

  return This;
}



/* Destructor of the object */
void __glcFontDestroy(__GLCfont *This, __GLCcontext* inContext)
{
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  if (This->faceDesc)
    __glcFaceDescDestroy(This->faceDesc, inContext);

  __glcFree(This);
}



/* Extract from the font the glyph which corresponds to the character code
 * 'inCode'.
 */
__GLCglyph* __glcFontGetGlyph(__GLCfont *This, GLint inCode,
			      __GLCcontext* inContext)
{
  /* Try to get the glyph from the character map */
  __GLCglyph* glyph = __glcCharMapGetGlyph(This->charMap, inCode);

  if (!glyph) {
    /* If it fails, we must extract the glyph from the face */
    glyph = __glcFaceDescGetGlyph(This->faceDesc, inCode, inContext);
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
GLfloat* __glcFontGetBoundingBox(__GLCfont *This, GLint inCode,
				 GLfloat* outVec, __GLCcontext* inContext,
				 GLfloat inScaleX, GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);

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
				   inContext)) {
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
GLfloat* __glcFontGetAdvance(__GLCfont* This, GLint inCode, GLfloat* outVec,
			     __GLCcontext* inContext, GLfloat inScaleX,
			     GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);

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
			       inScaleX, inScaleY, inContext)) {
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
GLfloat* __glcFontGetKerning(__GLCfont* This, GLint inCode, GLint inPrevCode,
			     GLfloat* outVec, __GLCcontext* inContext,
			     GLfloat inScaleX, GLfloat inScaleY)
{
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);
  __GLCglyph* prevGlyph = __glcFontGetGlyph(This, inPrevCode, inContext);

  if (!glyph || !prevGlyph)
    return NULL;

  return __glcFaceDescGetKerning(This->faceDesc, glyph->index, prevGlyph->index,
				 inScaleX, inScaleY, outVec, inContext);
}



/* This internal function tries to open the face file which name is identified
 * by 'inFace'. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __GLCfont object "inFont". Otherwise, it leaves the
 * font unchanged. GL_TRUE or GL_FALSE are returned to indicate if the function
 * succeeded or not.
 */
GLboolean __glcFontFace(__GLCfont* This, const GLCchar8* inFace,
			__GLCcontext *inContext)
{
  __GLCfaceDescriptor *faceDesc = NULL;
  __GLCmaster *master = NULL;
  __GLCcharMap* newCharMap = NULL;

  /* TODO : Verify if the font has already the required face activated */

  master = __glcMasterCreate(This->parentMasterID, inContext);
  if (!master) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  /* Get the face descriptor of the face identified by the string inFace */
  faceDesc = __glcFaceDescCreate(master, inFace, inContext);
  if (!faceDesc) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcMasterDestroy(master);
    return GL_FALSE;
  }

  newCharMap = __glcCharMapCreate(master, inContext);
  if (!newCharMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescDestroy(faceDesc, inContext);
    __glcMasterDestroy(master);
    return GL_FALSE;
  }

  __glcMasterDestroy(master);

#ifndef GLC_FT_CACHE
  /* If the font belongs to GLC_CURRENT_FONT_LIST then open the font file */
  if (FT_List_Find(&inContext->currentFontList, This)) {

    /* Open the new face */
    if (!__glcFaceDescOpen(faceDesc, inContext)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFaceDescDestroy(faceDesc, inContext);
      __glcCharMapDestroy(newCharMap);
      return GL_FALSE;
    }

    /* Close the current face */
    __glcFontClose(This);
  }
#endif

  /* Destroy the current charmap */
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  This->charMap = newCharMap;

  __glcFaceDescDestroy(This->faceDesc, inContext);
  This->faceDesc = faceDesc;

  return GL_TRUE;
}



#ifndef GLC_FT_CACHE
/* Open the font file */
void* __glcFontOpen(__GLCfont* This, __GLCcontext* inContext)
{
  return __glcFaceDescOpen(This->faceDesc, inContext);
}



/* Close the font file */
void __glcFontClose(__GLCfont* This)
{
   __glcFaceDescClose(This->faceDesc);
}
#endif
