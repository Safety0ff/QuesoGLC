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

/** \file
 *  defines the double-linked lists in a platform independent way.
 */

#include "list.h"
#include "ocontext.h"

#ifndef __WIN32__
typedef struct {
  void *user;
  void (*func)(void *data, void *user);
} userData;
#endif

GLboolean __glcCreateList(__glcList* list)
#ifndef __WIN32__
{
  *list = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  if (!(*list)) return GL_FALSE;
  (*list)->head = NULL;
  (*list)->tail = NULL;
  return GL_TRUE;
}
#else
{
  *list = (__glcWinList*)__glcMalloc(sizeof(__glcWinList));
  if (!(*list)) return GL_FALSE;
  InitializeListHead((*list)->entry);
  return GL_TRUE;
}
#endif

#ifndef __WIN32__
/* This function is called from FT_List_Finalize() to destroy all
 * remaining contexts
 */
static void __glcFTListDestructor(FT_Memory memory, void *data, void *user)
{
  userData *udat = (userData *)user;

  if (!udat)
    return;
  udat->func(data, udat->user);
}
#endif

void __glcListFinalize(__glcList list, void (*__glcDestructor)(void *data,
	void *user), void *user)
#ifndef __WIN32__
{
  userData udat;

  udat.user = user;
  udat.func = __glcDestructor;

  FT_List_Finalize(list, __glcFTListDestructor,
		   __glcCommonArea->memoryManager, &udat);
}
#else
{
}
#endif
