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

/* Defines the methods of an object that is intended to managed fonts */

#include "internal.h"
#include "ocontext.h"
#include "ofont.h"

__glcFont* __glcFontCreate(GLint inID, __glcMaster *inParent)
{
  __glcUniChar *s = __glcStrLstFindIndex(inParent->faceFileName, 0);
  GLCchar *buffer = NULL;
  __glcFont *This = NULL;
  __glcContextState *state = __glcGetCurrent();

  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  This = (__glcFont*)__glcMalloc(sizeof(__glcFont));

  This->faceID = 0;
  This->parent = inParent;
  This->charMapCount = 0;
  This->id = inID;

  buffer = (GLCchar*)__glcMalloc(__glcUniLenBytes(s));
  if (!buffer) {
    __glcFree(This);
    return NULL;
  }
  __glcUniDup(s, buffer, __glcUniLenBytes(s));

  if (FT_New_Face(state->library, 
		  (const char*)buffer, 0, &This->face)) {
    /* Unable to load the face file, however this should not happen since
       it has been succesfully loaded when the master was created */
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(buffer);
    __glcFree(This);
    return NULL;
  }

  __glcFree(buffer);

  /* select a Unicode charmap */
  if (FT_Select_Charmap(This->face, ft_encoding_unicode)) {
    /* Arrghhh, no Unicode charmap is available. This should not happen
       since it has been tested at master creation */
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Face(This->face);
    __glcFree(This);
    return NULL;
  }

  return This;
}

void __glcFontDestroy(__glcFont *This)
{
  FT_Done_Face(This->face);
  __glcFree(This);
}
