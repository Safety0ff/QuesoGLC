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

/* This file defines miscellaneous utility routines used throughout the lib */

#include <stdlib.h>
#include "ocontext.h"
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



/* FT list destructor 
 * This destructor should be used whenever the data field of a linked list
 * is used as an actual pointer : in QuesoGLC, some linked lists use the
 * data field to store an integer value, in which case, the destroy parameter
 * of FT_List_Finalize() must be set to NULL, otherwise a segmentation fault
 * will occur.
 */
void __glcListDestructor(FT_Memory inMemory, void *inData, void *inUser)
{
  __glcFree(inData);
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

/* Create an initialize a FreeType  double linked list */
GLboolean __glcCreateList(FT_List* list)
{
  *list = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  if (!(*list)) return GL_FALSE;
  (*list)->head = NULL;
  (*list)->tail = NULL;
  return GL_TRUE;
}

/* Duplicate a Unicode string */
__glcUniChar* __glcUniCopy(__glcUniChar* inUniChar, GLint inStringType)
{
  __glcUniChar* outUniChar = NULL;
  int length = 0;
  GLCchar* buffer = NULL;

  length = __glcUniEstimate(inUniChar, inStringType);
  buffer = (GLCchar *)__glcMalloc(length);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  __glcUniConvert(inUniChar, buffer, inStringType, length);
  outUniChar = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!outUniChar) {
    __glcFree(buffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  outUniChar->ptr = buffer;
  outUniChar->type = inStringType;

  return outUniChar;
}
