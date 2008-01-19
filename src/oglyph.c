/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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

/** \file
 * defines the object __GLCglyph which caches all the data needed for a given
 * glyph : display list, texture, bounding box, advance, index in the font
 * file, etc.
 */

#include "internal.h"
#include "texture.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the index of the glyph in the font file and its Unicode
 * codepoint.
 */
__GLCglyph* __glcGlyphCreate(GLCulong inIndex, GLCulong inCode)
{
  __GLCglyph* This = NULL;

  This = (__GLCglyph*)__glcMalloc(sizeof(__GLCglyph));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = This;

  This->index = inIndex;
  This->codepoint = inCode;
  This->isSpacingChar = GL_FALSE;
  This->textureObject = NULL;
  This->nContour = 0;
  This->contours = NULL;

  /* A display list for each rendering mode (except GLC_BITMAP) may be built */
  memset(This->displayList, 0, 4 * sizeof(GLuint));
  memset(This->bufferObject, 0, 3 * sizeof(GLuint));
  memset(This->boundingBox, 0, 4 * sizeof(GLfloat));
  memset(This->advance, 0, 2 * sizeof(GLfloat));

  This->advanceCached = GL_FALSE;
  This->boundingBoxCached = GL_FALSE;

  return This;
}



/* Destructor of the object */
void __glcGlyphDestroy(__GLCglyph* This, __GLCcontext* inContext)
{
  __glcGlyphDestroyGLObjects(This, inContext);
  __glcFree(This);
}



/* Remove all GL objects related to the texture of the glyph */
void __glcGlyphDestroyTexture(__GLCglyph* This, __GLCcontext* inContext)
{
  if (!inContext->isInGlobalCommand)
    glDeleteLists(This->displayList[1], 1);
  This->displayList[1] = 0;
  This->textureObject = NULL;
}



/* This function destroys the display lists and the texture objects that
 * are associated with a glyph.
 */
void __glcGlyphDestroyGLObjects(__GLCglyph* This, __GLCcontext* inContext)
{
  if (This->displayList[1]) {
    __glcDeleteAtlasElement(This->textureObject, inContext);
    __glcGlyphDestroyTexture(This, inContext);
  }

  if (!inContext->isInGlobalCommand) {
    if (This->displayList[0])
      glDeleteLists(This->displayList[0], 1);

    if (This->displayList[2])
      glDeleteLists(This->displayList[2], 1);

    if (This->displayList[3])
      glDeleteLists(This->displayList[3], 1);

    memset(This->displayList, 0, 4 * sizeof(GLuint));

    if (GLEW_ARB_vertex_buffer_object) {
      int i = 0;

      for (i = 0; i < 3; i++) {
	if (This->bufferObject[i])
	  glDeleteBuffersARB(1, &This->bufferObject[i]);
      }

      memset(This->bufferObject, 0, 3 * sizeof(GLuint));

      if (This->contours)
	__glcFree(This->contours);
      This->nContour = 0;
      This->contours = NULL;
    }
  }

}



/* Returns the number of display that has been built for a glyph */
int __glcGlyphGetDisplayListCount(__GLCglyph* This)
{
  int i = 0;
  int count = 0;

  for (i = 0; i < 4; i++) {
    if (This->displayList[i] && (This->displayList[i] != 0xffffffff))
      count++;
  }

  return count;
}



/* Returns the ID of the inCount-th display list that has been built for a
 * glyph.
 */
GLuint __glcGlyphGetDisplayList(__GLCglyph* This, int inCount)
{
  int i = 0;

  assert(inCount >= 0);
  assert(inCount < __glcGlyphGetDisplayListCount(This));

  for (i = 0; i < 4; i++) {
    GLuint displayList = This->displayList[i];

    if (displayList && (displayList != 0xffffffff)) {
      if (!inCount)
	return displayList;
      inCount--;
    }
  }

  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
  return 0xdeadbeef;
}



/* Returns the number of buffer objects that has been built for a glyph */
int __glcGlyphGetBufferObjectCount(__GLCglyph* This)
{
  int i = 0;
  int count = 0;

  for (i = 0; i < 3; i++) {
    if (This->bufferObject[i])
      count++;
  }

  return count;
}



/* Returns the ID of the inCount-th buffer object that has been built for a
 * glyph.
 */
GLuint __glcGlyphGetBufferObject(__GLCglyph* This, int inCount)
{
  int i = 0;

  assert(inCount >= 0);
  assert(inCount < __glcGlyphGetBufferObjectCount(This));

  for (i = 0; i < 3; i++) {
    GLuint bufferObject = This->bufferObject[i];

    if (bufferObject) {
      if (!inCount)
	return bufferObject;
      inCount--;
    }
  }

  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
  return 0xdeadbeef;
}
