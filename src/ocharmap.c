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

  if (inCharSet)
    This->charSet = FcCharSetCopy(inCharSet);
  else
    This->charSet = FcCharSetCreate();

  if (!This->charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->map = __glcArrayCreate(sizeof(__glcCharMapEntry));
  if (!This->map) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcCharSetDestroy(This->charSet);
    __glcFree(This);
    return NULL;
  }

  return This;
}



void __glcCharMapDestroy(__glcCharMap* This)
{
  if (This->map) {
    int i = 0;

    for (i = 0; i < GLC_ARRAY_LENGTH(This->map); i++)
      __glcCharMapDestroyGLObjects(This, i);

    __glcArrayDestroy(This->map);
  }

  FcCharSetDestroy(This->charSet);

  __glcFree(This);
}



static __glcCharMapEntry* __glcCharMapInsertCode(__glcCharMap* This,
						 GLint inCode,
						 GLint inMappedCode,
						 GLint inGlyph)
{
  __glcCharMapEntry* entry = NULL;
  __glcCharMapEntry* newEntry = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  while (start <= end) {
    middle = (start + end) >> 1;
    if (entry[middle].codepoint == inCode) {
      entry[middle].glyphIndex = inGlyph;
      entry[middle].mappedCode = inMappedCode;
      return &entry[middle];
    }
    else if (entry[middle].codepoint > inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  if ((end >= 0) && (entry[middle].codepoint < inCode))
    middle++;

  newEntry = (__glcCharMapEntry*)__glcArrayInsertCell(This->map, middle);
  if (!newEntry)
    return NULL;

  newEntry->codepoint = inCode;
  newEntry->glyphIndex = inGlyph;
  newEntry->mappedCode = inMappedCode;
  newEntry->textureObject = 0;
  memset(newEntry->displayList, 0, 3 * sizeof(GLuint));
  return newEntry;
}



void __glcCharMapAddChar(__glcCharMap* This, FT_UInt inGlyph,
			 GLint inCode, FT_ULong inMappedCode,
			 __glcContextState* inState)
{
  FcCharSet* charSet = NULL;

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

  if (!__glcCharMapInsertCode(This, inCode, inMappedCode, inGlyph)) {
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
  __glcCharMapEntry* entry = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (entry[middle].codepoint == inCode) {
      __glcCharMapDestroyGLObjects(This, middle);
      __glcArrayRemove(This->map, middle);
      break;
    }
    else if (entry[middle].codepoint > inCode)
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
  __glcCharMapEntry* entry = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  /* Look for the character which the character identifed by inCode is
   * mapped by */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (entry[middle].codepoint == inCode) {
      inCode = entry[middle].mappedCode;
      break;
    }
    else if (entry[middle].codepoint > inCode)
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



__glcCharMapEntry* __glcCharMapGetEntry(__glcCharMap* This,
					FT_UInt inGlyph,
					__glcContextState* inState,
					GLint inCode)
{
  __glcCharMapEntry* entry = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  /* Retrieve which is the glyph that inCode is mapped to */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (entry[middle].codepoint == inCode)
      return &entry[middle];
    else if (entry[middle].codepoint > inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  return __glcCharMapInsertCode(This, inCode, inCode, inGlyph);
}



GLboolean __glcCharMapHasChar(__glcCharMap* This, GLint inCode)
{
  __glcCharMapEntry* entry = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  /* Look for the character which the character identifed by inCode is
   * mapped by */
  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Look for the character mapped by inCode in the charmap */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (entry[middle].codepoint == inCode) {
      inCode = entry[middle].mappedCode;
      break;
    }
    else if (entry[middle].codepoint > inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  /* Check if the character identified by inCode exists in the font */
  return FcCharSetHasChar(This->charSet, inCode);
}



/* This function counts the number of bits that are set in c1 
 * Copied from Keith Packard's fontconfig
 */
static FcChar32 __glcCharSetPopCount(FcChar32 c1)
{
  /* hackmem 169 */
  FcChar32    c2 = (c1 >> 1) & 033333333333;
  c2 = c1 - c2 - ((c2 >> 1) & 033333333333);
  return (((c2 + (c2 >> 3)) & 030707070707) % 077);
}



GLCchar* __glcCharMapGetCharNameByIndex(__glcCharMap* This, GLint inIndex,
					__glcContextState* inState)
{
  int i = 0;
  int j = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  FcChar32 next = 0;
  FcChar32 base = FcCharSetFirstPage(This->charSet, map, &next);
  FcChar32 count = 0;
  FcChar32 value = 0;

  do {
    for (i = 0; i < FC_CHARSET_MAP_SIZE; i++) {
      value = __glcCharSetPopCount(map[i]);

      if (count + value >= inIndex + 1) {
	for (j = 0; j < 32; j++) {
	  if ((map[i] >> j) & 1) count++;
	  if (count == inIndex + 1) {
	    FcChar8* name = __glcNameFromCode(base + (i << 5) + j);
	    GLCchar* buffer = NULL;

	    if (!name) {
	      __glcRaiseError(GLC_PARAMETER_ERROR);
	      return GLC_NONE;
	    }

	    /* Performs the conversion */
	    buffer = __glcConvertFromUtf8ToBuffer(inState, name,
						  inState->stringType);
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



/* Get the maximum mapped code of a character set */
GLint __glcCharMapGetMaxMappedCode(__glcCharMap* This)
{
  FcChar32 base = 0;
  FcChar32 next = 0;
  FcChar32 prev_base = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;

  base = FcCharSetFirstPage(This->charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  do {
    prev_base = base;
    base = FcCharSetNextPage(This->charSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  for (i = FC_CHARSET_MAP_SIZE - 1; i >= 0; i--)
    if (map[i]) break;
  assert(i >= 0); /* If the map contains no char then
		   * something went wrong... */
  for (j = 31; j >= 0; j--)
    if ((map[i] >> j) & 1) break;
  return prev_base + (i << 5) + j;
}



/* Get the minimum mapped code of a character set */
GLint __glcCharMapGetMinMappedCode(__glcCharMap* This)
{
  FcChar32 base = 0;
  FcChar32 next = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;

  base = FcCharSetFirstPage(This->charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
    if (map[i]) break;
  assert(i < FC_CHARSET_MAP_SIZE); /* If the map contains no char then
				    * something went wrong... */
  for (j = 0; j < 32; j++)
    if ((map[i] >> j) & 1) break;
  return base + (i << 5) + j;
}

/* This functions destroys the display lists and the texture objects that
 * are associated with a charmap entry identified by inCharMapEntry.
 */
void __glcCharMapDestroyGLObjects(__glcCharMap* This, int inRank)
{
  __glcCharMapEntry* entry = (__glcCharMapEntry*)GLC_ARRAY_DATA(This->map);

  if (entry[inRank].displayList[0]) {
    glDeleteTextures(1, &entry[inRank].textureObject);
    glDeleteLists(entry[inRank].displayList[0], 1);
  }

  if (entry[inRank].displayList[1])
    glDeleteLists(entry[inRank].displayList[1], 1);

  if (entry[inRank].displayList[2])
    glDeleteLists(entry[inRank].displayList[2], 1);

  memset(entry[inRank].displayList, 0, 3 * sizeof(GLuint));
}

GLboolean __glcCharMapUnion(__glcCharMap* This, FcCharSet* inCharSet)
{
  FcCharSet* result = NULL;

  /* Add the character set to the charmap */
  result = FcCharSetUnion(This->charSet, inCharSet);
  if (!result) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  FcCharSetDestroy(This->charSet);
  This->charSet = result;

  return GL_TRUE;
}
