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

#include "ocharmap.h"
#include "internal.h"



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
  This->map = __glcArrayCreate(3 * sizeof(FT_ULong));

  return This;
}



void __glcCharMapDestroy(__glcCharMap* This)
{
  if (This->map)
    __glcArrayDestroy(This->map);

  FcCharSetDestroy(This->charSet);

  __glcFree(This);
}



static GLboolean __glcCharMapInsertCode(__glcCharMap* This, GLint inCode,
					GLint inMappedCode, GLint inGlyph)
{
  FT_ULong (*map)[3] = NULL;
  FT_ULong* data = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  map = (FT_ULong (*)[3])GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  while (start <= end) {
    middle = (start + end) >> 1;
    if (map[middle][0] == inCode) {
      map[middle][1] = inGlyph;
      map[middle][2] = inMappedCode;
      return GL_TRUE;
    }
    else if (map[middle][0] > inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  if ((end >= 0) && (map[middle][0] < inCode))
    middle++;

  data = (FT_ULong*)__glcArrayInsertCell(This->map, middle);
  if (!data)
    return GL_FALSE;

  data[0] = inCode;
  data[1] = inGlyph;
  data[2] = inMappedCode;
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
  FT_ULong (*map)[3] = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  map = (FT_ULong (*)[3])GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (map[middle][0] == inCode) {
      __glcArrayRemove(This->map, middle);
      break;
    }
    else if (map[middle][0] > inCode)
      end = middle - 1;
    else
      start = middle + 1;
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
  FT_ULong (*map)[3] = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  map = (FT_ULong (*)[3])GLC_ARRAY_DATA(This->map);

  /* Look for the character which the character identifed by inCode is
   * mapped by */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (map[middle][0] == inCode) {
      inCode = map[middle][2];
      break;
    }
    else if (map[middle][0] > inCode)
      end = middle - 1;
    else
      start = middle + 1;
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
  FT_UInt glyph = 0;
  FT_ULong (*map)[3] = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  map = (FT_ULong (*)[3])GLC_ARRAY_DATA(This->map);

  /* Retrieve which is the glyph that inCode is mapped to */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (map[middle][0] == inCode)
      return map[middle][1];
    else if (map[middle][0] > inCode)
      end = middle - 1;
    else
      start = middle + 1;
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
  FT_ULong (*map)[3] = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  map = (FT_ULong (*)[3])GLC_ARRAY_DATA(This->map);

  /* Look for the character which the character identifed by inCode is
   * mapped by */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (map[middle][0] == inCode) {
      inCode = map[middle][2];
      break;
    }
    else if (map[middle][0] > inCode)
      end = middle - 1;
    else
      start = middle + 1;
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
