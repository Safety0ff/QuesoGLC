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

/* Defines the methods of an object that is intended to managed fonts */

#include "internal.h"

__glcFont* __glcFontCreate(GLint inID, __glcMaster *inParent,
			   __glcContextState* state)
{
  __glcFont *This = NULL;

  assert(inParent);
  assert(state);

  This = (__glcFont*)__glcMalloc(sizeof(__glcFont));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  /* At font creation, the default face is the first one.
   * glcFontFace() can change the face.
   */
  This->faceDesc = (__glcFaceDescriptor*)inParent->faceList.head;

  This->charMap = __glcCharMapCreate(This->faceDesc);
  if (!This->charMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->parent = inParent;
  This->id = inID;

  return This;
}

void __glcFontDestroy(__glcFont *This)
{
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  __glcFree(This);
}
