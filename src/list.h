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

#ifndef __glc_list_h
#define __glc_list_h

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef __WIN32__
#include <windows.h>

typedef struct {
  LIST_ENTRY entry;
  void *data;
} __glcWinList;

#define __glcList __glcWinList*
#else
#include <ft2build.h>
#include FT_LIST_H

#define __glcList FT_List
#endif

GLboolean __glcCreateList(__glcList* list);
void __glcListFinalize(__glcList list, void (*__glcDestructor)(void *data,
	void *user), void *user);

#endif
