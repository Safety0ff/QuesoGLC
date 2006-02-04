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

#ifndef __glc_oarray_h
#define __glc_oarray_h

typedef struct {
  char* data;
  int allocated;
  int length;
  int elementSize;
} __glcArray;

__glcArray* __glcArrayCreate(int inElementSize);
void __glcArrayDestroy(__glcArray* This);
__glcArray* __glcArrayAppend(__glcArray* This, void* inValue);
__glcArray* __glcArrayInsert(__glcArray* This, int inRank, void* inValue);
void __glcArrayRemove(__glcArray* This, int inRank);
#endif
