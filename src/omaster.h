#ifndef __glc_omaster_h
#define __glc_omaster_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glc.h"
#include "constant.h"
#include "btree.h"
#include "strlst.h"

class __glcMaster {
 public:
  GLint id;
  __glcStringList* faceList;	/* GLC_FACE_LIST */
  FT_List charList;             /* GLC_CHAR_LIST */
  GLint charListCount;	        /* GLC_CHAR_LIST_COUNT */
  __glcUniChar* family;		/* GLC_FAMILY */
  __glcUniChar* masterFormat;	/* GLC_MASTER_FORMAT */
  __glcUniChar* vendor;		/* GLC_VENDOR */
  __glcUniChar* version;	/* GLC_VERSION */
  GLint isFixedPitch;		/* GLC_IS_FIXED_PITCH */
  GLuint maxMappedCode;	        /* GLC_MAX_MAPPED_CODE */
  GLuint minMappedCode;	        /* GLC_MIN_MAPPED_CODE */
  __glcStringList* faceFileName;
  BSTree* displayList;

  __glcMaster(FT_Face face, const char* inVendorName, const char* inFileExt, GLint inID, GLint inStringType);
  ~__glcMaster();
};

#endif /* __glc_omaster_h */
