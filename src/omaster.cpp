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

__glcMaster::__glcMaster(FT_Face face, const char* inVendorName, const char* inFileExt, GLint inID, GLint inStringType)
{
  static char format1[] = "Type1";
  static char format2[] = "True Type";
  __glcUniChar s;
  GLCchar *buffer = NULL;
  int length = 0;

  s.ptr = face->family_name;
  s.type = GLC_UCS1;

  vendor = NULL;
  faceList = NULL;
  faceFileName = NULL;
  family = NULL;
  masterFormat = NULL;
  buffer = NULL;

  faceList = __glcStrLstCreate(NULL);
  if (!faceList)
    goto error;

  faceFileName = __glcStrLstCreate(NULL);
  if (!faceFileName)
    goto error;

  /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
   * may be free and should be used instead of using the last location
   */
  length = __glcUniEstimate(&s, inStringType);
  buffer = (GLCchar *)__glcMalloc(length);
  if (!buffer)
    goto error;

  __glcUniConvert(&s, buffer, inStringType, length);
  family = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!family)
    goto error;
  family->ptr = buffer;
  family->type = inStringType;

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
    masterFormat = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
    masterFormat->ptr = buffer;
    masterFormat->type = inStringType;
    if (!masterFormat)
      goto error;
  }
  else
    masterFormat = NULL;

  s.ptr = (GLCchar*) inVendorName;
  length = __glcUniEstimate(&s, inStringType);
  buffer = (GLCchar*)__glcMalloc(length);
  if (!buffer)
    goto error;

  __glcUniConvert(&s, buffer, inStringType, length);
  vendor = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!vendor)
    goto error;
  vendor->ptr = buffer;
  vendor->type = inStringType;

  charList = (FT_List)__glcMalloc(sizeof(*charList));
  if (!charList)
    goto error;

  charList->head = NULL;
  charList->tail = NULL;

  version = NULL;
  isFixedPitch = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? GL_TRUE : GL_FALSE;
  charListCount = 0;
  minMappedCode = 0x7fffffff;
  maxMappedCode = 0;
  id = inID;
  displayList = NULL;
  return;

 error:
  if (vendor) {
    __glcUniDestroy(vendor);
    __glcFree(vendor);
  }
  if (faceList)
    delete faceList;
  if (faceFileName)
    delete faceFileName;
  if (family) {
    __glcUniDestroy(family);
    __glcFree(family);
  }
  if (masterFormat) {
    __glcUniDestroy(masterFormat);
    __glcFree(masterFormat);
  }
  if (buffer)
    __glcFree(buffer);
  __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  vendor = NULL;
  faceList = NULL;
  faceFileName = NULL;
  family = NULL;
  masterFormat = NULL;
  buffer = NULL;
  return;
}

__glcMaster::~__glcMaster()
{
  FT_List_Finalize(charList, __glcListDestructor, __glcContextState::memoryManager, NULL);
  __glcFree(charList);
  delete faceList;
  delete faceFileName;
  __glcUniDestroy(family);
  __glcUniDestroy(masterFormat);
  __glcUniDestroy(vendor);
  __glcFree(family);
  __glcFree(masterFormat);
  __glcFree(vendor);
    
  delete displayList;
}
