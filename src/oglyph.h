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

/** \file
 * header of the object __GLCglyph which caches all the data needed for a given
 * glyph : display list, texture, bounding box, advance, index in the font
 * file, etc.
 */

#ifndef __glc_oglyph_h
#define __glc_oglyph_h

typedef struct __GLCglyphRec __GLCglyph;
typedef struct __GLCatlasElementRec __GLCatlasElement;

struct __GLCglyphRec {
  FT_ListNodeRec node;

  GLCulong index;
  GLCulong codepoint;
  GLboolean isSpacingChar;
  /* GL objects management */
  __GLCatlasElement* textureObject;
  GLuint displayList[4];
  GLuint bufferObject[3];
  GLint nContour;
  GLint* contours;
  /* Measurement infos */
  GLfloat boundingBox[4];
  GLfloat advance[2];
  GLboolean advanceCached;
  GLboolean boundingBoxCached;
};

__GLCglyph* __glcGlyphCreate(GLCulong inIndex, GLCulong inCode);
void __glcGlyphDestroy(__GLCglyph* This, __GLCcontext* inContext);
void __glcGlyphDestroyTexture(__GLCglyph* This, __GLCcontext* inContext);
void __glcGlyphDestroyGLObjects(__GLCglyph* This, __GLCcontext* inContext);
int __glcGlyphGetDisplayListCount(__GLCglyph* This);
GLuint __glcGlyphGetDisplayList(__GLCglyph* This, int inCount);
int __glcGlyphGetBufferObjectCount(__GLCglyph* This);
GLuint __glcGlyphGetBufferObject(__GLCglyph* This, int inCount);
#endif
