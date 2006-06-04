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

#include <math.h>

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
    /* A Unicode codepoint can be no higher than 0x10ffff
     * (see Unicode specs)
     */
    if (inCode > 0x10ffff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    else {
      /* Codepoints lower or equal to 0x10ffff can be encoded on 4 bytes in
       * UTF-8 format
       */
      FcChar8 buffer[FC_UTF8_MAX_LEN];
#ifdef DEBUGMODE
      int len = FcUcs4ToUtf8((FcChar32)inCode, buffer);
#endif
      assert(len <= sizeof(GLint));

      return *((GLint*)buffer);
    }
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
    /* Convert the codepoint in UCS4 format and check if it is ill-formed or
     * not
     */
    if (FcUtf8ToUcs4((FcChar8*)&inCode, (FcChar32*)&code, sizeof(GLint)) < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
  }

  return code;
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



/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code and '<hexcode>' format
 * are issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__glcContextState *inState, GLint inCode,
		       __glcProcessCharFunc inProcessCharFunc,
		       void* inProcessCharData)
{
  GLint repCode = 0;
  GLint font = 0;

  /* Get a font that maps inCode */
  font = __glcCtxGetFont(inState, inCode);
  if (font >= 0) {
    /* A font has been found */
    return inProcessCharFunc(inCode, font, inState, inProcessCharData,
			     GL_FALSE);
  }

  /* __glcCtxGetFont() can not find a font that maps inCode, we then attempt to
   * produce an alternate rendering.
   */

  /* If the variable GLC_REPLACEMENT_CODE is nonzero, and __glcCtxGetFont()
   * finds a font that maps the replacement code, we now render the character
   * that the replacement code is mapped to
   */
  repCode = inState->replacementCode;
  font = __glcCtxGetFont(inState, repCode);
  if (repCode && font)
    return inProcessCharFunc(repCode, font, inState, inProcessCharData,
			     GL_FALSE);
  else {
    /* If we get there, we failed to render both the character that inCode maps
     * to and the replacement code. Now, we will try to render the character
     * sequence "\<hexcode>", where '\' is the character REVERSE SOLIDUS 
     * (U+5C), '<' is the character LESS-THAN SIGN (U+3C), '>' is the character
     * GREATER-THAN SIGN (U+3E), and 'hexcode' is inCode represented as a
     * sequence of hexadecimal digits. The sequence has no leading zeros, and
     * alphabetic digits are in upper case. The GLC measurement commands treat
     * the sequence as a single character.
     */
    char buf[10];
    GLint i = 0;
    GLint n = 0;

    /* Check if a font maps to '\', '<' and '>'. */
    if (!__glcCtxGetFont(inState, '\\') || !__glcCtxGetFont(inState, '<')
	|| !__glcCtxGetFont(inState, '>'))
      return NULL;

    /* Check if a font maps hexadecimal digits */
    sprintf(buf,"%X", (int)inCode);
    n = strlen(buf);
    for (i = 0; i < n; i++) {
      if (!__glcCtxGetFont(inState, buf[i]))
	return NULL;
    }

    /* Render the '\<hexcode>' sequence */
    inProcessCharFunc('\\', __glcCtxGetFont(inState, '\\'), inState,
			   inProcessCharData, GL_FALSE);
    inProcessCharFunc('<', __glcCtxGetFont(inState, '<'), inState,
			   inProcessCharData, GL_TRUE);
    for (i = 0; i < n; i++)
      inProcessCharFunc(buf[i], __glcCtxGetFont(inState, buf[i]), inState,
		      inProcessCharData, GL_TRUE);
    return inProcessCharFunc('>', __glcCtxGetFont(inState, '>'), inState,
			     inProcessCharData, GL_TRUE);
  }
}



static void __glcMakeIdentity(GLfloat* m)
{
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}



static GLboolean __glcInvertMatrix(GLfloat* inMatrix, GLfloat* outMatrix)
{
  int i, j, k, swap;
  GLfloat t;
  GLfloat temp[4][4];

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      temp[i][j] = inMatrix[i*4+j];
    }
  }
  __glcMakeIdentity(outMatrix);

  for (i = 0; i < 4; i++) {
    /* Look for largest element in column */
    swap = i;
    for (j = i + 1; j < 4; j++) {
      if (fabs(temp[j][i]) > fabs(temp[i][i])) {
	swap = j;
      }
    }

    if (swap != i) {
      /* Swap rows */
      for (k = 0; k < 4; k++) {
	t = temp[i][k];
	temp[i][k] = temp[swap][k];
	temp[swap][k] = t;

	t = outMatrix[i*4+k];
	outMatrix[i*4+k] = outMatrix[swap*4+k];
	outMatrix[swap*4+k] = t;
      }
    }

    if (fabs(temp[i][i]) < GLC_EPSILON) {
      /* No non-zero pivot. The matrix is singular, which shouldn't
       * happen. This means the user gave us a bad matrix.
       */
      return GL_FALSE;
    }

    t = temp[i][i];
    for (k = 0; k < 4; k++) {
      temp[i][k] /= t;
      outMatrix[i*4+k] /= t;
    }
    for (j = 0; j < 4; j++) {
      if (j != i) {
	t = temp[j][i];
	for (k = 0; k < 4; k++) {
	  temp[j][k] -= temp[i][k]*t;
	  outMatrix[j*4+k] -= outMatrix[i*4+k]*t;
	}
      }
    }
  }
  return GL_TRUE;
}



static void __glcMultMatrices(GLfloat* inMatrix1, GLfloat* inMatrix2,
			      GLfloat* outMatrix)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      outMatrix[i*4+j] = 
	inMatrix1[i*4+0]*inMatrix2[0*4+j] +
	inMatrix1[i*4+1]*inMatrix2[1*4+j] +
	inMatrix1[i*4+2]*inMatrix2[2*4+j] +
	inMatrix1[i*4+3]*inMatrix2[3*4+j];
    }
  }
}



/* Compute an optimal size for that glyph to be rendered on the screen if no
 * display list is planned to be built.
 */
GLboolean __glcGetScale(__glcContextState* inState,
			GLfloat* outTransformMatrix, GLfloat* outScaleX,
			GLfloat* outScaleY)
{
  GLint listIndex = 0;
  GLboolean displayListIsBuilding = GL_FALSE;
  int i = 0;

  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  displayListIsBuilding = listIndex || inState->glObjects;

  if (inState->renderStyle != GLC_BITMAP) {
    GLfloat projectionMatrix[16];
    GLfloat modelviewMatrix[16];
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

    /* Compute the matrix that transforms object space coordinates to viewport
     * coordinates. If we plan to use object space coordinates, this matrix is
     * set to identity.
     */
    __glcMultMatrices(modelviewMatrix, projectionMatrix, outTransformMatrix);

    if ((!displayListIsBuilding) && inState->hinting) {
      GLfloat rs[16], m[16];
      GLfloat sx = sqrt(outTransformMatrix[0] * outTransformMatrix[0]
			+outTransformMatrix[1] * outTransformMatrix[1]
			+outTransformMatrix[2] * outTransformMatrix[2]);
      GLfloat sy = sqrt(outTransformMatrix[4] * outTransformMatrix[4]
			+outTransformMatrix[5] * outTransformMatrix[5]
			+outTransformMatrix[6] * outTransformMatrix[6]);
      GLfloat sz = sqrt(outTransformMatrix[8] * outTransformMatrix[8]
			+outTransformMatrix[9] * outTransformMatrix[9]
			+outTransformMatrix[10] * outTransformMatrix[10]);
      GLfloat x = 0., y = 0.;

      memset(rs, 0, 16 * sizeof(GLfloat));
      rs[15] = 1.;
      for (i = 0; i < 3; i++) {
	rs[0+4*i] = outTransformMatrix[0+4*i] / sx;
	rs[1+4*i] = outTransformMatrix[1+4*i] / sy;
	rs[2+4*i] = outTransformMatrix[2+4*i] / sz;
      }
      if (!__glcInvertMatrix(rs, rs)) {
	*outScaleX = 0.f;
	*outScaleY = 0.f;
	return displayListIsBuilding;
      }

      __glcMultMatrices(rs, outTransformMatrix, m);
      x = ((m[0] + m[12])/(m[3] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[1] + m[13])/(m[3] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleX = sqrt(x*x+y*y);
      x = ((m[4] + m[12])/(m[7] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[5] + m[13])/(m[7] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleY = sqrt(x*x+y*y);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }
  else {
    GLfloat determinant = 0., norm = 0.;
    GLfloat *transform = inState->bitmapMatrix;

    /* Compute the norm of the transformation matrix */
    for (i = 0; i < 4; i++) {
      if (fabsf(transform[i]) > norm)
	norm = fabsf(transform[i]);
    }

    determinant = transform[0] * transform[3] - transform[1] * transform[2];

    /* If the transformation is degenerated, nothing needs to be rendered */
    if (fabsf(determinant) < norm * GLC_EPSILON) {
      *outScaleX = 0.f;
      *outScaleY = 0.f;
      return displayListIsBuilding;
    }

    if (inState->hinting) {
      *outScaleX = sqrt(transform[0]*transform[0]+transform[1]*transform[1]);
      *outScaleY = sqrt(transform[2]*transform[2]+transform[3]*transform[3]);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }

  return displayListIsBuilding;
}
