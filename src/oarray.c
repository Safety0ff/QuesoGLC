/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002,2004-2006, Bertrand Coconnier
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

#include "internal.h"

#define GLC_ARRAY_BLOCK_SIZE 16



__glcArray* __glcArrayCreate(int inElementSize)
{
  __glcArray* This = NULL;

  This = (__glcArray*)__glcMalloc(sizeof(__glcArray));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->data = (char*)__glcMalloc(GLC_ARRAY_BLOCK_SIZE * inElementSize);
  if (!This->data) {
    __glcFree(This);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->allocated = GLC_ARRAY_BLOCK_SIZE;
  This->length = 0;
  This->elementSize = inElementSize;

  return This;
}



void __glcArrayDestroy(__glcArray* This)
{
  if (This->data) {
    assert(This->allocated);
    __glcFree(This->data);
  }

  __glcFree(This);
}



static __glcArray* __glcArrayUpdateSize(__glcArray* This)
{
  if (This->length == This->allocated) {
    char* data = NULL;

    data = (char*)__glcRealloc(This->data,
	(This->allocated + GLC_ARRAY_BLOCK_SIZE) * This->elementSize);
    if (!data) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    This->data = data;
    This->allocated += GLC_ARRAY_BLOCK_SIZE;
  }

  return This;
}



__glcArray* __glcArrayAppend(__glcArray* This, void* inValue)
{
  if (!__glcArrayUpdateSize(This))
    return NULL;

  memcpy(This->data + This->length*This->elementSize, inValue,
	 This->elementSize);
  This->length++;

  return This;
}



__glcArray* __glcArrayInsert(__glcArray* This, int inRank, void* inValue)
{
  if (!__glcArrayUpdateSize(This))
    return NULL;

  if (This->length > inRank)
    memmove(This->data + (inRank+1) * This->elementSize,
    	    This->data + inRank * This->elementSize,
	   (This->length - inRank) * This->elementSize);

  memcpy(This->data + inRank*This->elementSize, inValue, This->elementSize);
  This->length++;

  return This;
}



void __glcArrayRemove(__glcArray* This, int inRank)
{
  if (inRank < This->length-1)
    memmove(This->data + inRank * This->elementSize,
	    This->data + (inRank+1) * This->elementSize,
	    (This->length - inRank - 1) * This->elementSize);
  This->length--;
}
