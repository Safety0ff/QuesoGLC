/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

#ifndef __glc_omaster_h
#define __glc_omaster_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glc.h"
#include "constant.h"
#include "ostrlst.h"

typedef struct {
  GLint id;
  FT_List faceList;	        /* GLC_FACE_LIST */
  FT_List charList;             /* GLC_CHAR_LIST */
  GLint charListCount;		/* GLC_CHAR_COUNT */
  __glcUniChar* family;		/* GLC_FAMILY */
  __glcUniChar* masterFormat;	/* GLC_MASTER_FORMAT */
  __glcUniChar* vendor;		/* GLC_VENDOR */
  __glcUniChar* version;	/* GLC_VERSION */
  GLint isFixedPitch;		/* GLC_IS_FIXED_PITCH */
  GLuint maxMappedCode;	        /* GLC_MAX_MAPPED_CODE */
  GLuint minMappedCode;	        /* GLC_MIN_MAPPED_CODE */
  FT_List faceFileName;
  FT_List displayList;
} __glcMaster;

__glcMaster* __glcMasterCreate(FT_Face face, const char* inVendorName,
			       const char* inFileExt, GLint inID,
			       GLint inStringType);
void __glcMasterDestroy(__glcMaster *This);

#endif /* __glc_omaster_h */
