#include "internal.h"
#include "omaster.h"
#include "ocontext.h"

__glcMaster::__glcMaster(FT_Face face, const char* inVendorName, const char* inFileExt, GLint inID, GLint inStringType)
{
    static char format1[] = "Type1";
    static char format2[] = "True Type";
    __glcUniChar s = __glcUniChar(face->family_name, GLC_UCS1);
    __glcContextState *state = __glcContextState::getCurrent();
    __glcUniChar sc = __glcUniChar(state->__glcBuffer, state->stringType);
    /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
     * may be free and should be used instead of using the last location
     */
    s.convert(state->__glcBuffer, state->inStringType, GLC_STRING_CHUNK);
    family = (GLCchar *) malloc(sc.lenBytes());
    if (!family) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
    memcpy(family, state->__glcBuffer, sc.lenBytes());

    faceList = new __glcStringList(NULL, inStringType);
    if (!faceList) {
      free(family);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }

    faceFileName = new __glcStringList(NULL, inStringType);
    if (!faceFileName) {
      delete faceList;
      free(family);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }

    charListCount = 0;
    /* use file extension to determine the face format */
    s.ptr = NULL;
    if (!strcmp(inFileExt, "pfa") || !strcmp(inFileExt, "pfb"))
      s.ptr = format1;
    if (!strcmp(inFileExt, "ttf") || !strcmp(inFileExt, "ttc"))
      s.ptr = format2;

    if (s.ptr) {
      s.convert(state->__glcBuffer, state->inStringType, GLC_STRING_CHUNK);
      masterFormat = (GLCchar *) malloc(sc.lenBytes());
      if (!masterFormat) {
	delete faceList;
	delete faceFileName;
	free(family);
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
      }
      memcpy(masterFormat, state->__glcBuffer, sc.lenBytes());
    }
    else
      masterFormat = NULL;

    s.ptr = inVendorName;
    s.convert(state->__glcBuffer, state->inStringType, GLC_STRING_CHUNK);
    vendor = (GLCchar *) malloc(sc.lenBytes());
    if (!vendor) {
      delete faceList;
      delete faceFileName;
      free(family);
      free(masterFormat);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
    memcpy(vendor, state->__glcBuffer, sc.lenBytes());
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
