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

#ifndef __glc_strlst_h
#define __glc_strlst_h

#include <string.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LIST_H
#include "GL/glc.h"
#include "ounichar.h"

#define GLC_STRING_CHUNK	256

FT_List __glcStrLstCreate(__glcUniChar* inString);
void __glcStrLstDestroy(FT_List This);
GLint __glcStrLstAppend(FT_List This, __glcUniChar* inString);
GLint __glcStrLstPrepend(FT_List This, __glcUniChar* inString);
GLint __glcStrLstRemove(FT_List This, __glcUniChar* inString);
GLint __glcStrLstRemoveIndex(FT_List This, GLuint inIndex);
__glcUniChar* __glcStrLstFindIndex(FT_List This, GLuint inIndex);
GLint __glcStrLstGetIndex(FT_List This, __glcUniChar* inString);
GLuint __glcStrLstLen(FT_List This);

#endif /* __glc_strlst_h */
