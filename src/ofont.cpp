/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
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
#include "internal.h"
#include "ocontext.h"
#include "ofont.h"

__glcFont::__glcFont(GLint inID, __glcMaster *inParent)
{
  __glcUniChar *s = inParent->faceFileName->findIndex(0);
  GLCchar *buffer = NULL;
  __glcContextState *state = __glcContextState::getCurrent();

  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  faceID = 0;
  parent = inParent;
  charMapCount = 0;
  id = inID;

  buffer = (GLCchar*)__glcMalloc(s->lenBytes());
  if (!buffer) {
    face = NULL;
    return;
  }
  s->dup(buffer, s->lenBytes());

  if (FT_New_Face(state->library, 
		  (const char*)buffer, 0, &face)) {
    /* Unable to load the face file, however this should not happen since
       it has been succesfully loaded when the master was created */
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    __glcFree(buffer);
    face = NULL;
    return;
  }

  __glcFree(buffer);

  /* select a Unicode charmap */
  if (FT_Select_Charmap(face, ft_encoding_unicode)) {
    /* Arrghhh, no Unicode charmap is available. This should not happen
       since it has been tested at master creation */
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    FT_Done_Face(face);
    face = NULL;
    return ;
  }
}

__glcFont::~__glcFont()
{
  FT_Done_Face(face);
}
