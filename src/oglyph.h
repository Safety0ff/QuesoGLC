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

#ifndef __glc_oglyph_h
#define __glc_oglyph_h

typedef struct {
  FT_ListNodeRec node;

  FT_ULong index;
  FT_ULong codepoint;
  /* GL objects management */
  void* textureObject;
  GLuint displayList[4];
  /* Measurement infos */
  GLfloat boundingBox[4];
  GLfloat advance[2];
} __glcGlyph;

__glcGlyph* __glcGlyphCreate(FT_ULong inIndex, FT_ULong inCode);
void __glcGlyphDestroy(__glcGlyph* This);
void __glcGlyphDestroyTexture(__glcGlyph* This);
void __glcGlyphDestroyGLObjects(__glcGlyph* This);
int __glcGlyphGetDisplayListCount(__glcGlyph* This);
GLuint __glcGlyphGetDisplayList(__glcGlyph* This, int inCount);
#endif
