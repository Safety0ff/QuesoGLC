/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2006, Bertrand Coconnier
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

/* Defines the methods of an object that is intended to managed glyphs */

#include "internal.h"

/** \file
 * defines the object __glcGlyph which caches all the data needed for a given
 * glyph : display list, texture, bounding box, advance, index in the font
 * file, etc.
 */

/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the index of the glyph in the font file and its Unicode
 * codepoint.
 */
__glcGlyph* __glcGlyphCreate(FT_ULong inIndex, FT_ULong inCode)
{
  __glcGlyph* This = NULL;

  This = (__glcGlyph*)__glcMalloc(sizeof(__glcGlyph));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = This;

  This->index = inIndex;
  This->codepoint = inCode;
  This->textureObject = 0;
  /* A display list for each rendering mode (except GLC_BITMAP) may be built */
  memset(This->displayList, 0, 3 * sizeof(GLuint));
  memset(This->boundingBox, 0, 4 * sizeof(GLfloat));
  memset(This->advance, 0, 2 * sizeof(GLfloat));

  return This;
}



/* Destructor of the object */
void __glcGlyphDestroy(__glcGlyph* This)
{
  __glcGlyphDestroyGLObjects(This);
  __glcFree(This);
}



/* This function destroys the display lists and the texture objects that
 * are associated with a glyph.
 */
void __glcGlyphDestroyGLObjects(__glcGlyph* This)
{
  if (This->displayList[0]) {
    glDeleteTextures(1, &This->textureObject);
    glDeleteLists(This->displayList[0], 1);
  }

  if (This->displayList[1])
    glDeleteLists(This->displayList[1], 1);

  if (This->displayList[2])
    glDeleteLists(This->displayList[2], 1);

  memset(This->displayList, 0, 3 * sizeof(GLuint));
}