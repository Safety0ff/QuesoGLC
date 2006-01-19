/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2006, Bertrand Coconnier
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

/* Defines the methods of an object that is intended to managed masters */

#include "internal.h"
#include FT_LIST_H

static const char unknown[] = "Unknown";

__glcMaster* __glcMasterCreate(const FcChar8* familyName,
			       const FcChar8* inVendorName,
			       GLint inID, GLint inStringType)
{
  __glcMaster *This = NULL;

  This = (__glcMaster*)__glcMalloc(sizeof(__glcMaster));
  if (!This)
    return NULL;

  This->vendor = NULL;
  This->family = NULL;
  This->masterFormat = NULL;

  This->faceList.head = NULL;
  This->faceList.tail = NULL;

  /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
   * may be free and should be used instead of using the last location
   */
  This->family = (FcChar8*)strdup((const char*)familyName);
  if (!This->family)
    goto error;

  /* We assume that the format of data strings in font files is GLC_UCS1 */
  if (inVendorName) {
    This->vendor = (FcChar8*)strdup((const char*)inVendorName);
    if (!This->vendor)
      goto error;
  }
  else
    This->vendor = (FcChar8*)unknown;

  This->charList = FcCharSetCreate();
  if (!This->charList)
    goto error;

  This->fullNameSGI = NULL;
  This->version = NULL;
  This->minMappedCode = 0x7fffffff;
  This->maxMappedCode = 0;
  This->id = inID;

  This->displayList.head = NULL;
  This->displayList.tail = NULL;
  This->textureObjectList.head = NULL;
  This->textureObjectList.tail = NULL;

  return This;

 error:
  if (This->charList)
    FcCharSetDestroy(This->charList);
  if (This->vendor) {
    __glcFree(This->vendor);
  }
  if (This->family) {
    __glcFree(This->family);
  }
  __glcRaiseError(GLC_RESOURCE_ERROR);

  __glcFree(This);
  return NULL;
}

/* FIXME :
 * Check that linked lists are empty before the pointer is freed
 */
void __glcMasterDestroy(__glcMaster *This)
{
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

  FT_List_Finalize(&This->displayList, __glcDisplayListDestructor,
		   &__glcCommonArea.memoryManager, NULL);
  FT_List_Finalize(&This->textureObjectList, __glcTextureObjectDestructor,
		   &__glcCommonArea.memoryManager, NULL);

  /* Don't use FT_List_Finalize here, since __glcFaceDescDestroy also destroys
   * the node itself.
   */
  node = This->faceList.head;
  while (node) {
    next = node->next;
    __glcFaceDescDestroy((__glcFaceDescriptor*)node);
    node = next;
  }

  FcCharSetDestroy(This->charList);
  if (This->family)
    __glcFree(This->family);
  if (This->vendor != (FcChar8*)unknown)
    __glcFree(This->vendor);
  if (This->fullNameSGI)
    __glcFree(This->fullNameSGI);
  if (This->version)
    __glcFree(This->version);

  __glcFree(This);
}
