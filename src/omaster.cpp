#include "internal.h"
#include "omaster.h"
#include "ocontext.h"
#include FT_LIST_H

__glcMaster::__glcMaster(FT_Face face, const char* inVendorName, const char* inFileExt, GLint inID, GLint inStringType)
{
  static char format1[] = "Type1";
  static char format2[] = "True Type";
  __glcUniChar s = __glcUniChar(face->family_name, GLC_UCS1);
  GLCchar *buffer = NULL;
  int length = 0;

  /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
   * may be free and should be used instead of using the last location
   */
  length = s.estimate(inStringType);
  buffer = (GLCchar *)__glcMalloc(length);
  if (!buffer) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }
  s.convert(buffer, inStringType, length);
  family = new __glcUniChar(buffer, inStringType);
  if (!family) {
    __glcFree(buffer);
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  faceList = new __glcStringList(NULL);
  if (!faceList) {
    family->destroy();
    delete family;
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  faceFileName = new __glcStringList(NULL);
  if (!faceFileName) {
    delete faceList;
    family->destroy();
    delete family;
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* use file extension to determine the face format */
  s = __glcUniChar(NULL, GLC_UCS1);
  if (!strcmp(inFileExt, "pfa") || !strcmp(inFileExt, "pfb"))
    s = __glcUniChar(format1, GLC_UCS1);
  if (!strcmp(inFileExt, "ttf") || !strcmp(inFileExt, "ttc"))
    s = __glcUniChar(format2, GLC_UCS1);

  if (s.len()) {
    length = s.estimate(inStringType);
    buffer = (GLCchar*)__glcMalloc(length);
    if (!buffer) {
      delete faceList;
      family->destroy();
      delete family;
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
    s.convert(buffer, inStringType, length);
    masterFormat = new __glcUniChar(buffer, inStringType);
    if (!masterFormat) {
      delete faceList;
      delete faceFileName;
      family->destroy();
      delete family;
      __glcFree(buffer);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }
  else
    masterFormat = NULL;

  s = __glcUniChar(inVendorName, GLC_UCS1);
  length = s.estimate(inStringType);
  buffer = (GLCchar*)__glcMalloc(length);
  if (!buffer) {
    delete faceList;
    delete faceFileName;
    family->destroy();
    delete family;
    masterFormat->destroy();
    delete masterFormat;
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  s.convert(buffer, inStringType, length);
  vendor = new __glcUniChar(buffer, inStringType);
  if (!vendor) {
    delete faceList;
    delete faceFileName;
    family->destroy();
    delete family;
    masterFormat->destroy();
    delete masterFormat;
    __glcFree(buffer);
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  charList = (FT_List)__glcMalloc(sizeof(*charList));
  if (!charList) {
    vendor->destroy();
    delete vendor;
    delete faceList;
    delete faceFileName;
    family->destroy();
    delete family;
    masterFormat->destroy();
    delete masterFormat;
    __glcFree(buffer);
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }
  charList->head = NULL;
  charList->tail = NULL;

  version = NULL;
  isFixedPitch = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? GL_TRUE : GL_FALSE;
  charListCount = 0;
  minMappedCode = 0x7fffffff;
  maxMappedCode = 0;
  id = inID;
  displayList = NULL;
}

__glcMaster::~__glcMaster()
{
  FT_List_Finalize(charList, __glcListDestructor, __glcContextState::memoryManager, NULL);
  __glcFree(charList);
  delete faceList;
  delete faceFileName;
  family->destroy();
  masterFormat->destroy();
  vendor->destroy();
  delete family;
  delete masterFormat;
  delete vendor;
    
  delete displayList;
}
