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

#ifndef __glc_omaster_h
#define __glc_omaster_h

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glc.h"
#include "constant.h"


typedef struct {
	FcChar8* fileName;
	FcChar8* styleName;
	int indexInFile;
	FcCharSet* charSet;
} __glcFaceDescriptor;

typedef struct {
  GLint id;
  FT_List faceList;	        /* GLC_FACE_LIST */
  FcCharSet* charList;          /* GLC_CHAR_LIST */
  FcChar8* family;		/* GLC_FAMILY */
  FcChar8* masterFormat;	/* GLC_MASTER_FORMAT */
  FcChar8* vendor;		/* GLC_VENDOR */
  FcChar8* version;	        /* GLC_VERSION */
  GLboolean isFixedPitch;	/* GLC_IS_FIXED_PITCH */
  GLuint maxMappedCode;	        /* GLC_MAX_MAPPED_CODE */
  GLuint minMappedCode;	        /* GLC_MIN_MAPPED_CODE */
  FT_List displayList;		/* GLC_LIST_OBJECT_LIST */
  FT_List textureObjectList;	/* GLC_TEXTURE_OBJECT_LIST */
} __glcMaster;

__glcMaster* __glcMasterCreate(const FcChar8* familyName,
			       const FcChar8* inVendorName,
			       const char* inFileExt, GLint inID,
			       GLboolean fixed, GLint inStringType);
void __glcMasterDestroy(__glcMaster *This);

#endif /* __glc_omaster_h */
