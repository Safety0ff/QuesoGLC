/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
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
#include "omaster.h"
#include "ocontext.h"
#include FT_LIST_H

__glcMaster* __glcMasterCreate(FT_Face face, const char* inVendorName,
			       const char* inFileExt, GLint inID,
			       GLint inStringType)
{
  static char format1[] = "Type1";
  static char format2[] = "True Type";
  __glcUniChar s;
  GLCchar *buffer = NULL;
  int length = 0;
  __glcMaster *This = NULL;

  This = (__glcMaster*)__glcMalloc(sizeof(__glcMaster));

  s.ptr = face->family_name;
  s.type = GLC_UCS1;

  This->vendor = NULL;
  This->faceList = NULL;
  This->faceFileName = NULL;
  This->family = NULL;
  This->masterFormat = NULL;
  buffer = NULL;

  This->faceList = __glcStrLstCreate(NULL);
  if (!This->faceList)
    goto error;

  This->faceFileName = __glcStrLstCreate(NULL);
  if (!This->faceFileName)
    goto error;

  /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
   * may be free and should be used instead of using the last location
   */
  length = __glcUniEstimate(&s, inStringType);
  buffer = (GLCchar *)__glcMalloc(length);
  if (!buffer)
    goto error;

  __glcUniConvert(&s, buffer, inStringType, length);
  This->family = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!This->family)
    goto error;
  This->family->ptr = buffer;
  This->family->type = inStringType;

  /* use file extension to determine the face format */
  s.ptr = NULL;
  if (!strcmp(inFileExt, "pfa") || !strcmp(inFileExt, "pfb"))
    s.ptr = format1;
  if (!strcmp(inFileExt, "ttf") || !strcmp(inFileExt, "ttc"))
    s.ptr = format2;

  if (__glcUniLen(&s)) {
    length = __glcUniEstimate(&s, inStringType);
    buffer = (GLCchar*)__glcMalloc(length);
    if (!buffer)
      goto error;

    __glcUniConvert(&s, buffer, inStringType, length);
    This->masterFormat = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    This->masterFormat->ptr = buffer;
    This->masterFormat->type = inStringType;
    if (!This->masterFormat)
      goto error;
  }
  else
    This->masterFormat = NULL;

  s.ptr = (GLCchar*) inVendorName;
  length = __glcUniEstimate(&s, inStringType);
  buffer = (GLCchar*)__glcMalloc(length);
  if (!buffer)
    goto error;

  __glcUniConvert(&s, buffer, inStringType, length);
  This->vendor = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!This->vendor)
    goto error;
  This->vendor->ptr = buffer;
  This->vendor->type = inStringType;

  This->charList = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  if (!This->charList)
    goto error;

  This->charList->head = NULL;
  This->charList->tail = NULL;

  This->version = NULL;
  This->isFixedPitch = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? GL_TRUE : GL_FALSE;
  This->charListCount = 0;
  This->minMappedCode = 0x7fffffff;
  This->maxMappedCode = 0;
  This->id = inID;
  This->displayList = NULL;
  return This;

 error:
  if (This->vendor) {
    __glcUniDestroy(This->vendor);
    __glcFree(This->vendor);
  }
  if (This->faceList)
    __glcStrLstDestroy(This->faceList);
  if (This->faceFileName)
    __glcStrLstDestroy(This->faceFileName);
  if (This->family) {
    __glcUniDestroy(This->family);
    __glcFree(This->family);
  }
  if (This->masterFormat) {
    __glcUniDestroy(This->masterFormat);
    __glcFree(This->masterFormat);
  }
  if (buffer)
    __glcFree(buffer);
  __glcRaiseError(GLC_RESOURCE_ERROR);

  __glcFree(This);
  return NULL;
}

void __glcMasterDestroy(__glcMaster *This)
{
  FT_List_Finalize(This->charList, __glcListDestructor,
		   __glcCommonArea->memoryManager, NULL);
  __glcFree(This->charList);
  __glcFree(This->faceList);
  __glcFree(This->faceFileName);
  __glcUniDestroy(This->family);
  __glcUniDestroy(This->masterFormat);
  __glcUniDestroy(This->vendor);
  __glcFree(This->family);
  __glcFree(This->masterFormat);
  __glcFree(This->vendor);

  __glcFree(This->displayList);
  __glcFree(This);
}
