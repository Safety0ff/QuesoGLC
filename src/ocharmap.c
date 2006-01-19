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

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "ocharmap.h"
#include "internal.h"
#include "ofont.h"

#define GLC_CHARMAP_BLOCKSIZE 16



__glcCharMap* __glcCharMapCreate(__glcFaceDescriptor* inFaceDesc)
{
  __glcCharMap* This = NULL;

  This = (__glcCharMap*)__glcMalloc(sizeof(__glcCharMap));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->faceDesc = inFaceDesc;
  This->charSet = FcCharSetCopy(inFaceDesc->charSet);
  if (!This->charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }
  This->map = NULL;
  This->count = 0;
  This->length = 0;

  return This;
}



void __glcCharMapDestroy(__glcCharMap* This)
{
  if (This->map)
    __glcFree(This->map);

  FcCharSetDestroy(This->charSet);

  __glcFree(This);
}



static GLboolean __glcCharMapInsertCode(__glcCharMap* This, GLint inCode,
					GLint inMappedCode, GLint inGlyph)
{
  GLint i = 0;

  /* FIXME : use a dichotomic algo instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][0] >= inCode)
      break;
  }

  if ((i == This->count) || (This->map[i][0] != inCode)) {
    FT_ULong (*charMapPtr)[3] = NULL;
    GLint charMapLen = 0;

    /* The character identified by inCharName is not yet registered, we add
     * it to the charmap.
     */
    if (This->count >= This->length) {
      charMapLen = This->length + GLC_CHARMAP_BLOCKSIZE;
      charMapPtr = (FT_ULong (*)[3])__glcRealloc(This->map,
					    sizeof(FT_ULong) * 3 * charMapLen);
      if (!charMapPtr) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return GL_FALSE;
      }

      This->map = charMapPtr;
      This->length = charMapLen;
    }
    if (This->count != i)
      memmove(&This->map[i+1][0], &This->map[i][0], 
	      (This->count - i) * 3 * sizeof(FT_ULong));
    This->count++;
    This->map[i][0] = inCode;
  }

  This->map[i][1] = inGlyph;
  This->map[i][2] = inMappedCode;
  return GL_TRUE;
}



void __glcCharMapAddChar(__glcCharMap* This, GLint inCode,
			 const GLCchar* inCharName, __glcContextState* inState)
{
  FT_ULong mappedCode  = 0;
  GLCchar* buffer = NULL;
  FT_UInt glyph = 0;
  FT_Face face = __glcFaceDescOpen(This->faceDesc, inState);
  FcCharSet* charSet = NULL;

  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  
  /* Convert the character name identified by inCharName into UTF-8 format. The
   * result is stored into 'buffer'. */
  buffer = __glcConvertToUtf8(inCharName, inState->stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(This->faceDesc);
    return;
  }

  /* Retrieve the Unicode code from its name */
  mappedCode = __glcCodeFromName(buffer);
  if (mappedCode == -1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFaceDescClose(This->faceDesc);
    __glcFree(buffer);
    return;
  }

  /* Verify that the character exists in the face */
  if (!FcCharSetHasChar(This->faceDesc->charSet, mappedCode)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFree(buffer);
    __glcFaceDescClose(This->faceDesc);
    return;
  }

  glyph = FcFreeTypeCharIndex(face, mappedCode);

  __glcFree(buffer);
  __glcFaceDescClose(This->faceDesc);

  /* We must verify that inCode is not in charSet yet, otherwise
   * FcCharSetAddChar() returns an error not because Fontconfig can not add
   * the character to the character set but because the character is ALREADY in
   * the character set. So in order to distinguish between those 2 errors
   * we have to check first if inCode belongs to charSet or not.
   */
  if (!FcCharSetHasChar(This->charSet, inCode)) {
    charSet = FcCharSetCopy(This->charSet);
    if (!charSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    if (!FcCharSetAddChar(charSet, inCode)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcCharSetDestroy(charSet);
      return;
    }
  }

  if (!__glcCharMapInsertCode(This, inCode, mappedCode, glyph)) {
    if (charSet)
      FcCharSetDestroy(charSet);
    return;
  }

  if (charSet) {
    FcCharSetDestroy(This->charSet);
    This->charSet = charSet;
  }
}



void __glcCharMapRemoveChar(__glcCharMap* This, GLint inCode)
{
  GLint i = 0;

  /* Look for the character mapped by inCode in the charmap */
  /* FIXME : use a dichotomic algo. instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][0] == (FT_ULong)inCode) {
      /* Remove the character mapped by inCode */
      if (i < This->count-1)
	memmove(&This->map[i][0], &This->map[i+1][0],
		(This->count-i-1) * 3 * sizeof(FT_ULong));
      This->count--;
      break;
    }
  }

  if (FcCharSetHasChar(This->charSet, inCode)) {
    FcCharSet* result = NULL;
    FcCharSet* dummy = FcCharSetCreate();

    if (!dummy) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    if (!FcCharSetAddChar(dummy, inCode)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcCharSetDestroy(dummy);
      return;
    }

    result = FcCharSetSubtract(This->charSet, dummy);
    FcCharSetDestroy(dummy);

    if (!result) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    FcCharSetDestroy(This->charSet);
    This->charSet = result;
  }
}



GLCchar* __glcCharMapGetCharName(__glcCharMap* This, GLint inCode,
				 __glcContextState* inState)
{
  GLCchar *buffer = NULL;
  FcChar8* name = NULL;
  GLint i = 0;

  /* Look for the character which the character identifed by inCode is
   * mapped by */
  /* FIXME : use a dichotomic algo instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][0] == (FT_ULong)inCode) {
      inCode = This->map[i][2];
      break;
    }
  }

  /* Check if the character identified by inCode exists in the font */
  if (!FcCharSetHasChar(This->charSet, inCode))
    return GLC_NONE;

  name = __glcNameFromCode(inCode);
  if (!name) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Convert the Unicode to the current string type */
  buffer = __glcConvertFromUtf8ToBuffer(inState, name, inState->stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  return buffer;
}



FT_UInt __glcCharMapGlyphIndex(__glcCharMap* This, FT_Face inFace, GLint inCode)
{
  GLint i = 0;
  FT_UInt glyph = 0;

  /* Retrieve which is the glyph that inCode is mapped to */
  /* TODO : use a dichotomic algo. instead */
  for (i = 0; i < This->count; i++) {
    if ((FT_ULong)inCode == This->map[i][0])
      return This->map[i][1];
  }

  assert(FcCharSetHasChar(This->charSet, inCode));

  /* Get and load the glyph which unicode code is identified by inCode */
  glyph = FcFreeTypeCharIndex(inFace, inCode);

  if (!__glcCharMapInsertCode(This, inCode, inCode, glyph))
    return 0;
  else
    return glyph;
}



GLboolean __glcCharMapHasChar(__glcCharMap* This, GLint inCode)
{
  int i = 0;
  /* Look for the character which the character identifed by inCode is
   * mapped by */
  /* FIXME : use a dichotomic algo instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][0] == (FT_ULong)inCode) {
      inCode = This->map[i][2];
      break;
    }
  }

  /* Check if the character identified by inCode exists in the font */
  return FcCharSetHasChar(This->charSet, inCode);
}



GLCchar* __glcCharMapGetCharNameByIndex(__glcCharMap* This, GLint inIndex,
					__glcContextState* inState)
{
  return __glcGetCharNameByIndex(This->faceDesc->charSet, inIndex, inState);
}



GLint __glcCharMapGetCount(__glcCharMap* This)
{
  return FcCharSetCount(This->faceDesc->charSet);
}



GLint __glcCharMapGetMaxMappedCode(__glcCharMap* This)
{
  return __glcGetMaxMappedCode(This->faceDesc->charSet);
}



GLint __glcCharMapGetMinMappedCode(__glcCharMap* This)
{
  return __glcGetMinMappedCode(This->faceDesc->charSet);
}
