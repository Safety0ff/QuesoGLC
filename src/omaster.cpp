#include "internal.h"
#include "omaster.h"
#include "ocontext.h"

__glcMaster::__glcMaster(FT_Face face, const char* inVendorName, const char* inFileExt, GLint inID, GLint inStringType)
{
    static char format1[] = "Type1";
    static char format2[] = "True Type";
    /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
     * may be free and should be used instead of using the last location
     */
    family = (GLCchar *)strdup((const char*)face->family_name);

    faceList = new __glcStringList(inStringType);
    if (!faceList) {
      free(family);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }

    faceFileName = new __glcStringList(inStringType);
    if (!faceFileName) {
      delete faceList;
      free(family);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }

    charListCount = 0;
    /* use file extension to determine the face format */
    masterFormat = NULL;
    if (!strcmp(inFileExt, "pfa") || !strcmp(inFileExt, "pfb"))
      masterFormat = format1;
    if (!strcmp(inFileExt, "ttf") || !strcmp(inFileExt, "ttc"))
      masterFormat = format2;
    vendor = (GLCchar *)strdup(inVendorName);
    version = NULL;
    isFixedPitch = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? GL_TRUE : GL_FALSE;
    minMappedCode = 0;
    maxMappedCode = 0x7fffffff;
    id = inID;
    displayList = NULL;
}

__glcMaster::~__glcMaster()
{
  delete faceList;
  delete faceFileName;
  free(family);
  free(vendor);
    
  delete displayList;
}
