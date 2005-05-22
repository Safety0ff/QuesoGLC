/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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

__glcCharMap* __glcCharMapCreate(FcCharSet* inCharSet)
{
  __glcCharMap* This = NULL;

  This = (__glcCharMap*)__glcMalloc(sizeof(__glcCharMap));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->charSet = inCharSet;
  This->map = NULL;
  This->count = 0;
  This->length = 0;

  return This;
}

void __glcCharMapDestroy(__glcCharMap* This)
{
  if (This->map)
    __glcFree(This->map);

  __glcFree(This);
}

void __glcCharMapAddChar(__glcCharMap* This, GLint inCode,
			 const GLCchar* inCharName, __glcContextState* inState)
{
  GLint i = 0;
  FT_ULong mappedCode  = 0;
  GLCchar* buffer = NULL;
  FcCharSet* charSet = NULL;
  FcCharSet* result = NULL;
  FT_ULong (*charMapPtr)[2] = NULL;
  GLint charMapLen = 0;

  /* Convert the character name identified by inCharName into the GLC_UCS1
   * format. The result is stored into 'buffer'. */
  buffer = __glcConvertFromUtf8ToBuffer(inState, inCharName,
					inState->stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Retrieve the Unicode code from its name */
  mappedCode = __glcCodeFromName(buffer);
  if (mappedCode == -1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Verify that the glyph exists in the face */
  if (!FcCharSetHasChar(This->charSet, inCode)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Remove the character identified by 'inCode' */
  charSet = FcCharSetCreate();
  if (!charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  if (!FcCharSetAddChar(charSet, inCode)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcCharSetDestroy(charSet);
    return;
  }

  result = FcCharSetSubtract(This->charSet, charSet);
  if (!result) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcCharSetDestroy(charSet);
    return;
  }
  FcCharSetDestroy(This->charSet);
  FcCharSetDestroy(charSet);
  This->charSet = result;

  /* Add the character identified by 'inCharName' to the list GLC_CHAR_LIST. */
/*  if (!FcCharSetAddChar(font->parent->charList, mappedCode)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }*/

  /* FIXME : use a dichotomic algo instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][0] >= mappedCode)
      break;
  }
  if ((i == This->count) || (This->map[i][0] != mappedCode)) {
    /* The character identified by inCharName is not yet registered, we add
     * it to the charmap.
     */
    if (This->count >= This->length) {
      charMapLen = This->length + GLC_CHARMAP_BLOCKSIZE;
      charMapPtr = (FT_ULong (*)[2])__glcRealloc(This->map,
					    sizeof(FT_ULong) * 2 * charMapLen);
      if (!charMapPtr) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }

      This->map = charMapPtr;
      This->length = charMapLen;
    }
    if (This->count != i)
      memmove(&This->map[i+1][0], &This->map[i][0], 
	      (This->count - i) * 2 * sizeof(GLint));
    This->count++;
    This->map[i][0] = mappedCode;
  }
  /* Stores the code which 'inCharName' must be mapped by */
  This->map[i][1] = inCode;
  /* FIXME : the master charList is not updated */
}

void __glcCharMapRemoveChar(__glcCharMap* This, GLint inCode)
{
  GLint i = 0;
  FT_ULong (*charMapPtr)[2] = NULL;
  
  /* Look for the character mapped by inCode in the charmap */
  /* FIXME : use a dichotomic algo. instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][1] == (FT_ULong)inCode) {
      /* Remove the character mapped by inCode */
      if (i < This->count-1)
	memmove(&This->map[i][0], &This->map[i+1][0],
		(This->count-i-1) * 2 * sizeof(GLint));
      This->count--;

      This->length = This->count;
      charMapPtr = (FT_ULong (*)[2])__glcRealloc(This->map,
				     sizeof(FT_ULong) * 2 * This->length);
      if (charMapPtr || This->length == 0)
	This->map = charMapPtr;

      break;
    }
  }
  return;
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
    if (This->map[i][1] == (FT_ULong)inCode) {
      inCode = This->map[i][0];
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

  /* Retrieve which is the glyph that inCode is mapped to */
  /* TODO : use a dichotomic algo. instead */
  for (i = 0; i < This->count; i++) {
    if ((FT_ULong)inCode == This->map[i][0]) {
      inCode = This->map[i][1];
      break;
    }
  }

  assert(FcCharSetHasChar(This->charSet, inCode));

  /* Get and load the glyph which unicode code is identified by inCode */
  return FcFreeTypeCharIndex(inFace, inCode);
}

GLboolean __glcCharMapHasChar(__glcCharMap* This, GLint inCode)
{
  int i = 0;
  /* Look for the character which the character identifed by inCode is
   * mapped by */
  /* FIXME : use a dichotomic algo instead */
  for (i = 0; i < This->count; i++) {
    if (This->map[i][1] == (FT_ULong)inCode) {
      inCode = This->map[i][0];
      break;
    }
  }

  /* Check if the character identified by inCode exists in the font */
  return FcCharSetHasChar(This->charSet, inCode);
}

GLCchar* __glcCharMapGetCharNameByIndex(__glcCharMap* This, GLint inIndex)
{
  int i = 0;
  int j = 0;
  FcChar32 base, next, map[FC_CHARSET_MAP_SIZE];
    
  base = FcCharSetFirstPage(This->charSet, map, &next);
  do {
    for (i = 0; i < FC_CHARSET_MAP_SIZE; i++) {
      FcChar32 count = 0;
      FcChar32 value = __glcCharSetPopCount(map[i]);

      if (count + value >= inIndex + 1) {
	for (j = 0; j < 32; j++) {
	  if ((map[i] >> j) & 1) count++;
	  if (count == inIndex + 1) {
	    FcChar8* name = __glcNameFromCode(base + (i << 5) + j);
	    GLCchar* buffer = NULL;
	    __glcContextState* state = __glcGetCurrent();
    
	    if (!name) {
	      __glcRaiseError(GLC_PARAMETER_ERROR);
	      return GLC_NONE;
	    }

	    /* Performs the conversion */
	    buffer = __glcConvertFromUtf8ToBuffer(state, name,
						  state->stringType);
	    if (!buffer) {
	      __glcRaiseError(GLC_RESOURCE_ERROR);
	      return GLC_NONE;
	    }

	    return buffer;
	  }
	}
      }
      count += value;  
    }
    base = FcCharSetNextPage(This->charSet, map, &next);
  } while (base != FC_CHARSET_DONE);
  /* The character has not been found */
  __glcRaiseError(GLC_PARAMETER_ERROR);
  return GLC_NONE;
}

GLint __glcCharMapGetCount(__glcCharMap* This)
{
  return FcCharSetCount(This->charSet);
}

GLint __glcCharMapGetMaxMappedCode(__glcCharMap* This)
{
  return __glcGetMaxMappedCode(This->charSet);
}

GLint __glcCharMapGetMinMappedCode(__glcCharMap* This)
{
  return __glcGetMinMappedCode(This->charSet);
}
