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

/* Defines the methods of an object that is intended to managed masters */

#include "internal.h"
#include FT_LIST_H

static const char unknown[] = "Unknown";

/** \file
 * defines the object __glcFont which manage the fonts
 */

/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the family name and the vendor name of the fonts. 'inID'
 * contains the ID of the new master and 'inStringType' the string type of the
 * current context.
 */
__glcMaster* __glcMasterCreate(const FcChar8* familyName,
			       const FcChar8* inVendorName,
			       GLint inID, GLint inStringType)
{
  __glcMaster *This = NULL;

  This = (__glcMaster*)__glcMalloc(sizeof(__glcMaster));
  if (!This)
    return NULL;

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;

  This->vendor = NULL;
  This->family = NULL;
  This->masterFormat = NULL;

  This->faceList.head = NULL;
  This->faceList.tail = NULL;

  /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
   * may be freed and should be used instead of using the last location
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

  This->charList = __glcCharMapCreate(NULL);
  if (!This->charList)
    goto error;

  This->fullNameSGI = NULL;
  This->version = NULL;
  This->id = inID;

  return This;

 error:
  if (This->charList)
    __glcCharMapDestroy(This->charList);
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



/* Destructor of the object */
void __glcMasterDestroy(__glcMaster *This, __glcContextState* inState)
{
  /* FIXME :
   * Check that linked lists are empty before the pointer is freed
   */
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

  /* Don't use FT_List_Finalize here, since __glcFaceDescDestroy also destroys
   * the node itself.
   */
  node = This->faceList.head;
  while (node) {
    next = node->next;
    __glcFaceDescDestroy((__glcFaceDescriptor*)node, inState);
    node = next;
  }

  __glcCharMapDestroy(This->charList);
  if (This->family)
    __glcFree(This->family);
  if (This->vendor != (FcChar8*)unknown)
    __glcFree(This->vendor);

  if (This->fullNameSGI) {
    if (strncmp((const char*)This->fullNameSGI, "Unknown", 7))
      __glcFree(This->fullNameSGI);
  }
  if (This->version) {
    if (strncmp((const char*)This->version, "Unknown", 7))
      __glcFree(This->version);
  }

  __glcFree(This);
}
