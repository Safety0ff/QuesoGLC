/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

/* Defines the methods of an object that is intended to managed string lists
 * This file and the object it defines will be removed in a (hopefully) near
 * future. Double linked lists defined by FreeType will be used instead.
 */

#include "internal.h"
#include "ostrlst.h"

__glcStringList* __glcStrLstCreate(__glcUniChar* inString)
{
  GLCchar *room = NULL;
  __glcStringList *This = NULL;

  This = (__glcStringList*)__glcMalloc(sizeof(__glcStringList));

  if (inString) {
    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    __glcUniDup(inString, room, __glcUniLenBytes(inString));
    This->string = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    This->string->ptr = room;
    This->string->type = GLC_UCS1;
    This->count = 1;
  }
  else {
    This->string = NULL;
    This->count = 0;
  }
  This->next = NULL;

  return This;
}

void __glcStrLstDestroy(__glcStringList *This)
{
  if (This->next)
    __glcStrLstDestroy(This->next);

  if (This->string) {
    __glcUniDestroy(This->string);
    __glcFree(This->string);
  }

  __glcFree(This);
}

GLint __glcStrLstAppend(__glcStringList *This, __glcUniChar* inString)
{
  __glcStringList *item = This;
  __glcStringList *current = This;
  GLCchar *room = NULL;

  if (!inString)
    return -1;

  if (This->count) {
    __glcUniChar *s = NULL;

    if (inString->type > This->string->type)
      if (__glcStrLstConvert(This, inString->type))
	return -1;

    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    if (!room)
      return -1;

    do {
      item->count++;
      current = item;
      item = item->next;
    } while (item);

    __glcUniConvert(inString, room, current->string->type,
		    __glcUniLenBytes(inString));
    s = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    s->ptr = room;
    s->type = current->string->type;
    item = __glcStrLstCreate(s);

    __glcFree(s);
    __glcFree(room);

    if (!item) {
      item = This;
      do {
	item->count--;
	item = item->next;
      } while (item);
      return -1;
    }

    current->next = item;
  }
  else {
    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    if (!room)
      return -1;
    __glcUniDup(inString, room, __glcUniLenBytes(inString));
    This->string = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    This->string->ptr = room;
    This->string->type = inString->type;
    This->count = 1;
  }

  return 0;
}

GLint __glcStrLstPrepend(__glcStringList *This, __glcUniChar* inString)
{
  __glcStringList *item = NULL;
  __glcUniChar* temp = NULL;
  GLCchar *room = NULL;

  if (This->count) {
    if (inString->type > This->string->type)
      if (__glcStrLstConvert(This, inString->type))
	return -1;

    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    if (!room)
      return -1;
    __glcUniConvert(inString, room, This->string->type,
		    __glcUniLenBytes(inString));
    temp = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    temp->ptr = room;
    temp->type = This->string->type;
    item = __glcStrLstCreate(temp);

    __glcFree(room);
    __glcFree(temp);
    if (!item)
      return -1;

    temp = This->string;
    This->string = item->string;
    item->string = temp;
    item->next = This->next;
    This->next = item;
    item->count = This->count++;
  }
  else {
    room = (GLCchar *)__glcMalloc(__glcUniLenBytes(inString));
    if (!room)
      return -1;
    __glcUniDup(inString, room, __glcUniLenBytes(inString));
    This->string = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    This->string->ptr = room;
    This->string->type = inString->type;
    This->count = 1;
  }

  return 0;
}

GLint __glcStrLstRemove(__glcStringList *This, __glcUniChar* inString)
{
  return __glcStrLstRemoveIndex(This, __glcStrLstGetIndex(This, inString));
}

GLint __glcStrLstRemoveIndex(__glcStringList *This, GLuint inIndex)
{
  __glcStringList *list = NULL;

  /* String not found */
  if ((inIndex >= This->count) || (inIndex < 0))
    return -1;

  if (!inIndex) {
    __glcFree(This->string->ptr);
    __glcFree(This->string);
    
    if (This->next) {
      list = This->next;
      This->string = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
      This->string = list->string;
      list->string->ptr = NULL;
      This->count = list->count;
      This->next = list->next;
      __glcStrLstDestroy(list);
    }
    else {
      This->string = NULL;
      This->count = 0;
    }
  }
  else {
    GLuint index = 0;
    __glcStringList *current = This;
    list = This;

    while (index != inIndex) {
      list->count--;
      current = list;
      list = list->next;
      index++;
    }
    current->next = list->next;
    __glcStrLstDestroy(list);
  }

  return 0;
}

__glcUniChar* __glcStrLstFind(__glcStringList *This, __glcUniChar* inString)
{
  return __glcStrLstFindIndex(This, __glcStrLstGetIndex(This, inString));
}

__glcUniChar* __glcStrLstFindIndex(__glcStringList *This, GLuint inIndex)
{
  __glcStringList *item = This;
  GLuint index = 0;

  if ((inIndex >= This->count) || (inIndex < 0))
    return NULL;

  while(index != inIndex) {
    item = item->next;
    index++;
  };

  return item->string;
}

GLint __glcStrLstGetIndex(__glcStringList *This, __glcUniChar* inString)
{
  __glcStringList *item = This;
  int index = 0;

  if (!This->string)
    return -1;

  do {
    if (!__glcUniCompare(item->string, inString))
      break;
    item = item->next;
    index++;
  } while (item);

  if (!item)
    return -1;

  return index;
}

GLint __glcStrLstConvert(__glcStringList *This, int inType)
{
  GLCchar *room = NULL;
  int size = 0;
  __glcStringList *item = This;

  if (inType == This->string->type)
    return 0;

  do {
    size = __glcUniEstimate(item->string, inType);
    room = (GLCchar*)__glcMalloc(size);
    if (!room)
      return -1;

    __glcUniConvert(item->string, room, inType, size);
    __glcFree(item->string->ptr);
    item->string->ptr = room;
    item->string->type = inType;
    item = item->next;
  } while(item);

  return 0;
}
