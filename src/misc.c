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

/* This file defines miscellaneous utility routines used throughout the lib */

#include "internal.h"
#include FT_LIST_H



/* QuesoGLC own allocation and memory management routines */
void* __glcMalloc(size_t size)
{
  return malloc(size);
}

void __glcFree(void *ptr)
{
  free(ptr);
}

void* __glcRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}



/* Find a token in a list of tokens separated by 'separator' */
GLCchar* __glcFindIndexList(const GLCchar* inString, GLuint inIndex,
			    const GLCchar* inSeparator)
{
    GLuint i = 0;
    GLuint occurence = 0;
    char *s = (char *)inString;
    char *sep = (char *)inSeparator;

    if (!inIndex)
	return (GLCchar *)inString;

    /* TODO use Unicode instead of ASCII */
    for (i=0; i<=strlen(s); i++) {
	if (s[i] == *sep)
	    occurence++;
	if (occurence == inIndex) {
	    i++;
	    break;
	}
    }


    return (GLCchar *)&s[i];
}

/* Find a Unicode name from its code */
GLCchar* __glcNameFromCode(GLint code)
{
  GLint position = -1;

  if ((code < 0) || (code > __glcMaxCode))
    return GLC_NONE;

  position = __glcNameFromCodeArray[code];
  if (position == -1)
    return GLC_NONE;

  return __glcCodeFromNameArray[position].name;
}

/* Find a Unicode code from its name */
GLint __glcCodeFromName(GLCchar* name)
{
  int start = 0;
  int end = __glcCodeFromNameSize;
  int middle = (end + start) / 2;
  int res = 0;

  while (end - start > 1) {
    res = strcmp(name, __glcCodeFromNameArray[middle].name);
    if (res > 0)
      start = middle;
    else if (res < 0)
      end = middle;
    else
      return __glcCodeFromNameArray[middle].code;
    middle = (end + start) / 2;
  }
  if (strcmp(name, __glcCodeFromNameArray[start].name) == 0)
    return __glcCodeFromNameArray[start].code;
  if (strcmp(name, __glcCodeFromNameArray[end].name) == 0)
    return __glcCodeFromNameArray[end].code;
  return -1;
}



/* Convert a character from UCS1 to UTF-8 and return the number of bytes
 * needed to encode the char.
 */
static int __glcUcs1ToUtf8(FcChar8 ucs1, FcChar8 dest[FC_UTF8_MAX_LEN])
{
  FcChar8 *d = dest;

  if (ucs1 < 0x80)
    *d++ = ucs1;
  else {
    *d++ = ((ucs1 >> 6) & 0x1F) | 0xC0;
    *d++ = (ucs1 & 0x3F) | 0x80;
  }

  return d - dest;
}

/* Convert a character from UCS2 to UTF-8 and return the number of bytes
 * needed to encode the char.
 */
static int __glcUcs2ToUtf8(FcChar16 ucs2, FcChar8 dest[FC_UTF8_MAX_LEN])
{
  FcChar8 *d = dest;

  if (ucs2 < 0x80)
    *d++ = ucs2;
  else if (ucs2 < 0x800) {
    *d++ = ((ucs2 >> 6) & 0x1F) | 0xC0;
    *d++ = (ucs2 & 0x3F) | 0x80;
  }
  else {
    *d++ = ((ucs2 >> 12) & 0x0F) | 0xE0;
    *d++ = ((ucs2 >> 6) & 0x3F) | 0x80;
    *d++ = (ucs2 & 0x3F) | 0x80;
  }

  return d - dest;
}

static int __glcUtf8ToUcs1(const FcChar8* src_orig,
			   FcChar8 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  FcChar32 result = 0;
  int src_shift = FcUtf8ToUcs4(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x100) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      /* According to GLC specs, when the value of a character code exceed the
       * range of the character encoding, the returned character is converted
       * to a character sequence \<hexcode> where 'hexcode' is the original
       * character code represented as a sequence of hexadecimal digits
       */
      snprintf((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* Excluding the terminating '\0' character */
      *dstlen = strlen((const char*)dst) - 1;
    }
  }
  return src_shift;
}

static int __glcUtf8ToUcs2(const FcChar8* src_orig,
			   FcChar16 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  FcChar32 result = 0;
  int src_shift = FcUtf8ToUcs4(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x10000) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      int count;
      char* src = NULL;
      char buffer[GLC_OUT_OF_RANGE_LEN];

      /* According to GLC specs, when the value of a character code exceed the
       * range of the character encoding, the returned character is converted
       * to a character sequence \<hexcode> where 'hexcode' is the original
       * character code represented as a sequence of hexadecimal digits
       */
      snprintf(buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      for (count = 0, src = buffer; src && count < GLC_OUT_OF_RANGE_LEN;
	   count++, *dst++ = *src++);
      *dst = 0; /* Terminating '\0' character */
      *dstlen = count;
    }
  }
  return src_shift;
}

/* Convert 'inString' in the UTF-8 format and return a copy of the
 * converted string.
 */
FcChar8* __glcConvertToUtf8(const GLCchar* inString, const GLint inStringType)
{
  FcChar8 buffer[FC_UTF8_MAX_LEN];
  FcChar8* string = NULL;
  FcChar8* ptr = NULL;
  int len;

  switch(inStringType) {
  case GLC_UCS1:
    {
      FcChar8* ucs1 = NULL;

      for (len = 0, ucs1 = (FcChar8*)inString; *ucs1;
	     len += __glcUcs1ToUtf8(*ucs1++, buffer));
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      for (ucs1 = (FcChar8*)inString, ptr = string; *ucs1;
	     ptr += __glcUcs1ToUtf8(*ucs1++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS2:
    {
      FcChar16* ucs2 = NULL;

      for (len = 0, ucs2 = (FcChar16*)inString; *ucs2;
	     len += __glcUcs2ToUtf8(*ucs2++, buffer));
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      for (ucs2 = (FcChar16*)inString, ptr = string; *ucs2;
	     ptr += __glcUcs2ToUtf8(*ucs2++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS4:
    {
      FcChar32* ucs4 = NULL;

      for (len = 0, ucs4 = (FcChar32*)inString; *ucs4;
	     len += FcUcs4ToUtf8(*ucs4++, buffer));
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      for (ucs4 = (FcChar32*)inString, ptr = string; *ucs4;
	     ptr += FcUcs4ToUtf8(*ucs4++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UTF8_QSO:
    string = (FcChar8*)strdup((const char*)inString);
    break;
  default:
    return NULL;
  }

  return string;
}

/* Convert 'inString' from the UTF-8 format and return a copy of the
 * converted string.
 */
GLCchar* __glcConvertFromUtf8(const FcChar8* inString,
			      const GLint inStringType)
{
  GLCchar* string = NULL;
  const FcChar8* utf8 = NULL;
  int len_buffer = 0;
  int len = 0;
  int shift = 0;

  switch(inStringType) {
  case GLC_UCS1:
    {
      FcChar8 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar8* ucs1 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs1(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      string = (GLCchar*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      utf8 = inString;
      ucs1 = (FcChar8*)string;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs1(utf8, ucs1, strlen((const char*)utf8),
				&len_buffer);
	ucs1 += len_buffer;
      }
      *ucs1 = 0;
    }
    break;
  case GLC_UCS2:
    {
      FcChar16 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar16* ucs2 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs2(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      string = (GLCchar*)__glcMalloc((len+1)*sizeof(FcChar16));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      utf8 = inString;
      ucs2 = (FcChar16*)string;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs2(utf8, ucs2, strlen((const char*)utf8),
				&len_buffer);
	ucs2 += len_buffer;
      }
      *ucs2 = 0;
    }
    break;
  case GLC_UCS4:
    {
      FcChar32 buffer = 0;
      FcChar32* ucs4 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = FcUtf8ToUcs4(utf8, &buffer, strlen((const char*)utf8));
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len++;
      }

      string = (GLCchar*)__glcMalloc((len+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      utf8 = inString;
      ucs4 = (FcChar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0;
    }
    break;
  case GLC_UTF8_QSO:
    string = strdup((const char*)inString);
    break;
  default:
    return NULL;
  }
  return string;
}


/* Convert 'inString' from the UTF-8 format and return a copy of the
 * converted string in the context buffer.
 */
GLCchar* __glcConvertFromUtf8ToBuffer(__glcContextState* This,
				      const FcChar8* inString,
				      const GLint inStringType)
{
  GLCchar* string = NULL;
  const FcChar8* utf8 = NULL;
  int len_buffer = 0;
  int len = 0;
  int shift = 0;

  if (!inString)
    return GLC_NONE;

  switch(inStringType) {
  case GLC_UCS1:
    {
      FcChar8 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar8* ucs1 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs1(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs1 = (FcChar8*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs1(utf8, ucs1, strlen((const char*)utf8),
				&len_buffer);
	ucs1 += len_buffer;
      }

      *ucs1 = 0;
    }
    break;
  case GLC_UCS2:
    {
      FcChar16 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar16* ucs2 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs2(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar16));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs2 = (FcChar16*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs2(utf8, ucs2, strlen((const char*)utf8),
				&len_buffer);
	ucs2 += len_buffer;
      }
      *ucs2 = 0;
    }
    break;
  case GLC_UCS4:
    {
      FcChar32 buffer = 0;
      FcChar32* ucs4 = NULL;

      utf8 = inString;
      while(*utf8) {
	shift = FcUtf8ToUcs4(utf8, &buffer, strlen((const char*)utf8));
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len++;
      }

      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }


      utf8 = inString;
      ucs4 = (FcChar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0;
    }
    break;
  case GLC_UTF8_QSO:
    string = (GLCchar*)__glcCtxQueryBuffer(This,
					   strlen((const char*)inString)+1);
    strcpy(string, (const char*)inString);
    break;
  default:
    return NULL;
  }
  return string;
}



/* This function is called to destroy texture objects */
void __glcTextureObjectDestructor(FT_Memory inMemory, void *inData,
				  void *inUser)
{
  /* Hack in order to be able to store a 32 bits texture ID in a 32/64 bits
   * void* pointer so that we do not need to allocate memory just to store
   * a single integer value
   */
  union {
    void* ptr;
    GLuint ui;
  } voidToGLuint;

  voidToGLuint.ptr = inData;

  glDeleteTextures(1, &voidToGLuint.ui);
}



/* This function is called to destroy display lists */
void __glcDisplayListDestructor(FT_Memory inMemory, void *inData,
				  void *inUser)
{
  /* Hack in order to be able to store a 32 bits display list ID in a 32/64 bits
   * void* pointer so that we do not need to allocate memory just to store
   * a single integer value
   */
  union {
    void* ptr;
    GLuint ui;
  } voidToGLuint;

  voidToGLuint.ptr = inData;

  glDeleteLists(voidToGLuint.ui, 1);
}



/* This function counts the number of bits that are set in c1 
 * Copied from Keith Packard's fontconfig
 */
FcChar32 __glcCharSetPopCount(FcChar32 c1)
{
  /* hackmem 169 */
  FcChar32    c2 = (c1 >> 1) & 033333333333;
  c2 = c1 - c2 - ((c2 >> 1) & 033333333333);
  return (((c2 + (c2 >> 3)) & 030707070707) % 077);
}

/* Convert 'inCount' characters of 'inString' in the UTF-8 format and return a
 * copy of the converted string.
 */
FcChar8* __glcConvertCountedStringToUtf8(const GLint inCount,
					 const GLCchar* inString,
					 const GLint inStringType)
{
  FcChar8 buffer[FC_UTF8_MAX_LEN];
  FcChar8* string = NULL;
  FcChar8* ptr = NULL;
  int len = 0;
  int i = 0;

  switch(inStringType) {
  case GLC_UCS1:
    {
      FcChar8* ucs1 = NULL;

      len = 0;
      ucs1 = (FcChar8*)inString;
      for (i = 0; i < inCount; i++)
	len += __glcUcs1ToUtf8(*ucs1++, buffer);

      string = (FcChar8*)__glcMalloc(len*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs1 = (FcChar8*)inString;
      ptr = string;
      for (i = 0; i < inCount; i++)
	ptr += __glcUcs1ToUtf8(*ucs1++, ptr);
    }
    break;
  case GLC_UCS2:
    {
      FcChar16* ucs2 = NULL;

      len = 0;
      ucs2 = (FcChar16*)inString;
      for (i = 0; i < inCount; i++)
	len += __glcUcs2ToUtf8(*ucs2++, buffer);

      string = (FcChar8*)__glcMalloc(len*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs2 = (FcChar16*)inString;
      ptr = string;
      for (i = 0; i < inCount; i++)
	ptr += __glcUcs2ToUtf8(*ucs2++, ptr);
    }
    break;
  case GLC_UCS4:
    {
      FcChar32* ucs4 = NULL;

      len = 0;
      ucs4 = (FcChar32*)inString;
      for (i = 0; i < inCount; i++)
	len += FcUcs4ToUtf8(*ucs4++, buffer);

      string = (FcChar8*)__glcMalloc(len*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs4 = (FcChar32*)inString;
      ptr = string;
      for (i = 0; i < inCount; i++)
	ptr += FcUcs4ToUtf8(*ucs4++, ptr);
    }
    break;
  case GLC_UTF8_QSO:
    {
      FcChar8* utf8 = (FcChar8*)inString;
      FcChar32 dummy = 0;
      int len = 0;
      int shift = 0;

      for (i = 0; i < inCount; i++) {
	shift += FcUtf8ToUcs4(utf8, &dummy, 6);
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	len += shift;
      }

      string = (FcChar8*)__glcMalloc(len*sizeof(FcChar8));
      strncpy((char*)string, (const char*)inString, len);
    }
    break;
  default:
    return NULL;
  }

  return string;
}

/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. If the code can not be converted to the current string
 * type a GLC_PARAMETER_ERROR is issued.
 */
GLint __glcConvertUcs4ToGLint(__glcContextState *inState, GLint inCode)
{
  switch(inState->stringType) {
  case GLC_UCS2:
    /* Check that inCode can be stored in UCS-2 format */
    if (inCode <= 65535)
      break;
  case GLC_UCS1:
    /* Check that inCode can be stored in UCS-1 format */
    if (inCode <= 255)
      break;
  case GLC_UTF8_QSO:
    /* Things can go wrong if GLC_UTF8_QSO is the expected string type :
     * the length of some character code in UTF-8 format can exceed the size
     * of the GLint type. Hence whenever the string type is GLC_UTF8_QSO, this
     * function issues a GLC_PARAMETER_ERROR.
     * This is a limitation of the GLC API as it is defined in the specs
     * version 0.2. The most obvious workaround is to use the UCS-4/UTF-32
     * format...
     */
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return -1;
  }

  return inCode;
}

/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character codes
 * in GLint which may cause problems for the UTF-8 format.
 */
GLint __glcConvertGLintToUcs4(__glcContextState *inState, GLint inCode)
{
  GLint code = inCode;

  /* If the current string type is GLC_UTF8_QSO then converts to GLC_UCS4 */
  if (inState->stringType == GLC_UTF8_QSO) {
    /* Things can go wrong if GLC_UTF8_QSO is the expected string type :
     * the length of some character code in UTF-8 format can exceed the size
     * of the GLint type. Hence whenever the string type is GLC_UTF8_QSO, this
     * function issues a GLC_PARAMETER_ERROR.
     * This is a limitation of the GLC API as it is defined in the specs
     * version 0.2. The most obvious workaround is to use the UCS-4/UTF-32
     * format...
     */
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return -1;
  }

  return code;
}



/* This functions destroys the display lists and the texture objects that
 * are associated with a context identified by inState. The corresponding
 * linked lists are also deleted.
 */
void __glcDeleteGLObjects(__glcContextState *inState)
{
  FT_ListNode node = NULL;
  __glcMaster* master = NULL;

  /* Delete display lists and texture objects */
  for(node = inState->masterList.head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    FT_List_Finalize(&master->displayList, __glcDisplayListDestructor,
                     &__glcCommonArea.memoryManager, NULL);
    FT_List_Finalize(&master->textureObjectList, __glcTextureObjectDestructor,
		     &__glcCommonArea.memoryManager, NULL);
  }
}



/* Get the minimum mapped code of a character set */
GLint __glcGetMinMappedCode(FcCharSet *charSet)
{
  FcChar32 base = 0;
  FcChar32 next = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;

  base = FcCharSetFirstPage(charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
    if (map[i]) break;
  assert(i < FC_CHARSET_MAP_SIZE); /* If the map contains no char then
				    * something went wrong... */
  for (j = 0; j < 32; j++)
    if ((map[i] >> j) & 1) break;
  return base + (i << 5) + j;
}



/* Get the maximum mapped code of a character set */
GLint __glcGetMaxMappedCode(FcCharSet *charSet)
{
  FcChar32 base = 0;
  FcChar32 next = 0;
  FcChar32 prev_base = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;

  base = FcCharSetFirstPage(charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  do {
    prev_base = base;
    base = FcCharSetNextPage(charSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  for (i = FC_CHARSET_MAP_SIZE - 1; i >= 0; i--)
    if (map[i]) break;
  assert(i >= 0); /* If the map contains no char then
		   * something went wrong... */
  for (j = 31; j >= 0; j--)
    if ((map[i] >> j) & 1) break;
  return prev_base + (i << 5) + j;
}



/* Each thread has to store specific informations so they can retrieved later.
 * __glcGetThreadArea() returns a struct which contains thread specific info
 * for GLC. Notice that even if the lib does not support threads, this function
 * must be used.
 * If the 'threadArea' of the current thread does not exist, it is created and
 * initialized.
 * IMPORTANT NOTE : __glcGetThreadArea() must never use __glcMalloc() and
 *    __glcFree() since those functions could use the exceptContextStack
 *    before it is initialized.
 */
threadArea* __glcGetThreadArea(void)
{
  threadArea *area = NULL;

  area = (threadArea*)pthread_getspecific(__glcCommonArea.threadKey);

  if (!area) {
    area = (threadArea*)malloc(sizeof(threadArea));
    if (!area)
      return NULL;

    area->currentContext = NULL;
    area->errorState = GLC_NONE;
    area->lockState = 0;
    area->exceptionStack.head = NULL;
    area->exceptionStack.tail = NULL;
    area->failedTry = GLC_NO_EXC;
    pthread_setspecific(__glcCommonArea.threadKey, (void*)area);
  }

  return area;
}



/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcRaiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  /* An error can only be raised if the current error value is GLC_NONE. 
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = area->errorState;
  if ((inError && !error) || !inError)
    area->errorState = inError;
}



/* Get the current context of the issuing thread */
__glcContextState* __glcGetCurrent(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  return area->currentContext;
}



GLCchar* __glcGetCharNameByIndex(FcCharSet* inCharSet, GLint inIndex,
				 __glcContextState* inState)
{
  int i = 0;
  int j = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  FcChar32 next = 0;
  FcChar32 base = FcCharSetFirstPage(inCharSet, map, &next);
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
    base = FcCharSetNextPage(inCharSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  /* The character has not been found */
  __glcRaiseError(GLC_PARAMETER_ERROR);
  return GLC_NONE;
}
