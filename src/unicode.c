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

/* This file defines miscellaneous utility routines used for Unicode management */

#include <fribidi/fribidi.h>

#include "internal.h"



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



/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
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
      /* Convert to the string '\<xxx>' */
      snprintf((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* Excluding the terminating '\0' character */
      *dstlen = strlen((const char*)dst) - 1;
    }
  }
  return src_shift;
}



/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
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
      /* Convert to the string '\<xxx>' */
      int count;
      char* src = NULL;
      char buffer[GLC_OUT_OF_RANGE_LEN];

      snprintf(buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      for (count = 0, src = buffer; src && count < GLC_OUT_OF_RANGE_LEN;
	   count++, *dst++ = *src++);
      *dst = 0; /* Terminating '\0' character */
      *dstlen = count;
    }
  }
  return src_shift;
}



/* Convert 'inString' in the UTF-8 format and return a copy of the converted
 * string.
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

      /* Determine the length of the final string */
      for (len = 0, ucs1 = (FcChar8*)inString; *ucs1;
	     len += __glcUcs1ToUtf8(*ucs1++, buffer));
      /* Allocate the room to store the final string */
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs1 = (FcChar8*)inString, ptr = string; *ucs1;
	     ptr += __glcUcs1ToUtf8(*ucs1++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS2:
    {
      FcChar16* ucs2 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs2 = (FcChar16*)inString; *ucs2;
	     len += __glcUcs2ToUtf8(*ucs2++, buffer));
      /* Allocate the room to store the final string */
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs2 = (FcChar16*)inString, ptr = string; *ucs2;
	     ptr += __glcUcs2ToUtf8(*ucs2++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UCS4:
    {
      FcChar32* ucs4 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs4 = (FcChar32*)inString; *ucs4;
	     len += FcUcs4ToUtf8(*ucs4++, buffer));
      /* Allocate the room to store the final string */
      string = (FcChar8*)__glcMalloc((len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs4 = (FcChar32*)inString, ptr = string; *ucs4;
	     ptr += FcUcs4ToUtf8(*ucs4++, ptr));
      *ptr = 0;
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
    string = (FcChar8*)strdup((const char*)inString);
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

  assert(inString);

  switch(inStringType) {
  case GLC_UCS1:
    {
      FcChar8 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar8* ucs1 = NULL;

      /* Determine the length of the final string */
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

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      ucs1 = (FcChar8*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs1(utf8, ucs1, strlen((const char*)utf8),
				&len_buffer);
	ucs1 += len_buffer;
      }

      *ucs1 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      FcChar16 buffer[GLC_OUT_OF_RANGE_LEN];
      FcChar16* ucs2 = NULL;

      /* Determine the length of the final string */
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

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar16));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      ucs2 = (FcChar16*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs2(utf8, ucs2, strlen((const char*)utf8),
				&len_buffer);
	ucs2 += len_buffer;
      }
      *ucs2 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      FcChar32 buffer = 0;
      FcChar32* ucs4 = NULL;

      /* Determine the length of the final string */
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

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcCtxQueryBuffer(This, (len+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }


      /* Perform the conversion */
      utf8 = inString;
      ucs4 = (FcChar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
    string = (GLCchar*)__glcCtxQueryBuffer(This,
					   strlen((const char*)inString)+1);
    strcpy(string, (const char*)inString);
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
  switch(inState->stringState.stringType) {
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
  FcBlanks* blanks = NULL;

  if (inCode < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return -1;
  }

  /* If the current string type is GLC_UTF8_QSO then converts to GLC_UCS4 */
  if (inState->stringState.stringType == GLC_UTF8_QSO) {
    /* Convert the codepoint in UCS4 format and check if it is ill-formed or
     * not
     */
    if (FcUtf8ToUcs4((FcChar8*)&inCode, (FcChar32*)&code, sizeof(GLint)) < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
  }

  /* Treat all blank characters as '\0' */
  blanks = FcConfigGetBlanks(NULL);
  if (blanks) {
    if (FcBlanksIsMember(blanks, code))
      return 0;
  }

  return code;
}



/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
FcChar32* __glcConvertToVisualUcs4(__glcContextState* inState,
				   const GLCchar* inString)
{
  FcChar32* string = NULL;
  int length = 0;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  FcChar32* visualString = NULL;

  assert(inString);

  switch(inState->stringState.stringType) {
  case GLC_UCS1:
    {
      FcChar8* ucs1 = (FcChar8*)inString;
      FcChar32* ucs4 = NULL;

      length = strlen((const char*)ucs1);

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(length+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      for (ucs4 = string; *ucs1; ucs1++, ucs4++)
	*ucs4 = (FcChar32)(*ucs1);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      FcChar16* ucs2 = NULL;
      FcChar32* ucs4 = NULL;

      for (ucs2 = (FcChar16*)inString; *ucs2; ucs2++, length++);

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(length+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      for (ucs2 = (FcChar16*)inString, ucs4 = string; *ucs2; ucs2++, ucs4++)
	*ucs4 = (FcChar32)(*ucs2);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      FcChar32* ucs4 = NULL;
      int length = 0;

      for (ucs4 = (FcChar32*)inString; *ucs4; ucs4++, length++);

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(length+1)*sizeof(int));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      memcpy(string, inString, length*sizeof(int));

      ((int*)string)[length] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      FcChar32* ucs4 = NULL;
      FcChar8* utf8 = NULL;
      FcChar32 buffer = 0;
      int shift = 0;

      /* Determine the length of the final string */
      utf8 = (FcChar8*)inString;
      while(*utf8) {
	shift = FcUtf8ToUcs4(utf8, &buffer, strlen((const char*)utf8));
	if (shift < 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  __glcRaiseError(GLC_PARAMETER_ERROR);
	  return NULL;
	}
	utf8 += shift;
	length++;
      }

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(length+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      utf8 = (FcChar8*)inString;
      ucs4 = (FcChar32*)string;
      while(*utf8)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  visualString = string + length + 1;
  if (!fribidi_log2vis(string, length, &base, visualString, NULL, NULL, NULL)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  return visualString;
}



/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
FcChar32* __glcConvertCountedStringToVisualUcs4(__glcContextState* inState,
						const GLCchar* inString,
						const GLint inCount)
{
  FcChar32* string = NULL;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  FcChar32* visualString = NULL;

  assert(inString);

  switch(inState->stringState.stringType) {
  case GLC_UCS1:
    {
      FcChar8* ucs1 = (FcChar8*)inString;
      FcChar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(inCount+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs4 = string;

      for (i = 0; i < inCount; i++)
	*(ucs4++) = (FcChar32)(*(ucs1++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      FcChar16* ucs2 = NULL;
      FcChar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(inCount+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      ucs2 = (FcChar16*)inString;
      ucs4 = string;

      for (i = 0 ; i < inCount; i++)
	*(ucs4++) = (FcChar32)(*(ucs2++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(inCount+1)*sizeof(int));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      memcpy(string, inString, inCount*sizeof(int));

      ((int*)string)[inCount] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      FcChar32* ucs4 = NULL;
      FcChar8* utf8 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (FcChar32*)__glcCtxQueryBuffer(inState, 2*(inCount+1)*sizeof(FcChar32));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      utf8 = (FcChar8*)inString;
      ucs4 = (FcChar32*)string;

      for (i = 0; i < inCount; i++)
	utf8 += FcUtf8ToUcs4(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  visualString = string + inCount + 1;
  if (!fribidi_log2vis(string, inCount, &base, visualString, NULL, NULL, NULL)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  return visualString;
}
