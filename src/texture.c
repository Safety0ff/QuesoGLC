/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
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

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#define GLCAPI __declspec(dllexport)
#endif

#include "internal.h"

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "texture.h"
#include FT_OUTLINE_H
#include FT_LIST_H

#define GLC_TEXTURE_SIZE 64


/* Compute the lower power of 2 that is greater than value. It is used to
 * determine the smaller texture than can contain a glyph.
 */
static int __glcNextPowerOf2(int value)
{
  int power = 0;

  for (power = 1; power < value; power <<= 1);

  return power;
}



void __glcDeleteAtlasElement(__glcAtlasElement* This,
			     __glcContextState* inState)
{
  FT_List_Remove(&inState->atlasList, (FT_ListNode)This);
  inState->atlasCount--;
  free(This);
}



static GLboolean __glcTextureAtlasGetPosition(__glcContextState* inState,
					      __glcGlyph* inGlyph)
{
  __glcAtlasElement* atlasNode = NULL;

  if (!inState->atlas.id) {
    int size = 1024;
    int i = 0;
    GLint format = 0;
    GLint level = 0;

    for (i = 0; i < 3; i++) {
      glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, size,
		   size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			       &format);
      if (format)
        break;

      size >>= 1;
    }

    if (i == 3) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GL_FALSE;
    }

    glGenTextures(1, &inState->atlas.id);
    inState->atlas.width = size;
    inState->atlas.heigth = size;
    inState->atlasWidth = size / GLC_TEXTURE_SIZE;
    inState->atlasHeight = size / GLC_TEXTURE_SIZE;
    inState->atlasCount = 0;
    glBindTexture(GL_TEXTURE_2D, inState->atlas.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, size,
		 size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);

    while (size > 1) {
      size >>= 1;
      level++;
      glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA8, size,
		   size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
    }

    if (inState->enableState.mipmap)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		      GL_LINEAR_MIPMAP_LINEAR);
    else
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		      GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  }
  else
    glBindTexture(GL_TEXTURE_2D, inState->atlas.id);

  if (inState->atlasCount == inState->atlasWidth * inState->atlasHeight) {
    atlasNode = (__glcAtlasElement*)inState->atlasList.tail;
    assert(atlasNode);
    __glcGlyphDestroyTexture(atlasNode->glyph);
    FT_List_Up(&inState->atlasList, (FT_ListNode)atlasNode);
  }
  else {
    atlasNode = (__glcAtlasElement*)__glcMalloc(sizeof(__glcAtlasElement));
    if (!atlasNode) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GL_FALSE;
    }

    atlasNode->node.data = atlasNode;
    atlasNode->position = inState->atlasCount++;
    FT_List_Insert(&inState->atlasList, (FT_ListNode)atlasNode);
  }

  atlasNode->glyph = inGlyph;
  inGlyph->textureObject = atlasNode;

  return GL_TRUE;
}



static GLboolean __glcTextureGetImmediate(__glcContextState* inState,
					  GLsizei inWidth, GLsizei inHeight)
{
  GLint format = 0;

  if (inState->texture.id) {
    if ((inWidth > inState->texture.width)
	|| (inHeight > inState->texture.heigth)) {
      glDeleteTextures(1, &inState->texture.id);
      inState->texture.id = 0;
      inState->texture.width = 0;
      inState->texture.heigth = 0;
    }
    else {
      glBindTexture(GL_TEXTURE_2D, inState->texture.id);
      return GL_TRUE;
    }
  }

  /* Check if a new texture can be created */
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, inWidth,
	       inHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			   &format);
  if (!format) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  /* Create a texture object and make it current */
  glGenTextures(1, &inState->texture.id);
  glBindTexture(GL_TEXTURE_2D, inState->texture.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, inWidth,
	       inHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  inState->texture.width = inWidth;
  inState->texture.heigth = inHeight;

  return GL_TRUE;
}

/* Internal function that renders glyph in textures :
 * 'inCode' must be given in UCS-4 format
 */
void __glcRenderCharTexture(__glcFont* inFont,
			    __glcContextState* inState,
			    GLboolean inDisplayListIsBuilding,
			    GLfloat scale_x, GLfloat scale_y,
			    __glcGlyph* inGlyph)
{
  FT_Face face = NULL;
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_BBox mipmapBoundingBox;
  FT_Bitmap pixmap;
  FT_Matrix matrix;
  GLuint texture = 0;
  GLfloat width = 0, heigth = 0;
  GLint level = 0;
  GLint posX = 0, posY = 0;
  int minSize = (GLEW_VERSION_1_2 || GLEW_SGIS_texture_lod) ? 2 : 1;
  GLfloat texWidth = 0, texHeigth = 0;
  GLfloat texScaleX = 0, texScaleY = 0;

#ifndef FT_CACHE_H
  face = inFont->faceDesc->face;
  assert(face);
#else
  if (FTC_Manager_LookupFace(inState->cache, (FTC_FaceID)inFont->faceDesc,
			     &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
#endif

  outline = face->glyph->outline;

  if (inDisplayListIsBuilding) {
    if (!__glcTextureAtlasGetPosition(inState, inGlyph))
      return;

    matrix.xx = (FT_Fixed)((GLC_TEXTURE_SIZE << 16) / scale_x);
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = (FT_Fixed)((GLC_TEXTURE_SIZE << 16) / scale_y);

    FT_Outline_Transform(&outline, &matrix);
  }

  /* Get the bounding box of the glyph */
  FT_Outline_Get_CBox(&outline, &boundingBox);

  /* Compute the size of the pixmap where the glyph will be rendered */
  if (inDisplayListIsBuilding) {
    __glcAtlasElement* atlasNode = (__glcAtlasElement*)inGlyph->textureObject;

    pixmap.width = GLC_TEXTURE_SIZE;
    pixmap.rows = GLC_TEXTURE_SIZE;
    texture = inState->atlas.id;
    texWidth = inState->atlas.width;
    texHeigth = inState->atlas.heigth;
    texScaleX = (GLfloat)GLC_TEXTURE_SIZE;
    texScaleY = (GLfloat)GLC_TEXTURE_SIZE;
    posY = (atlasNode->position / inState->atlasWidth);
    posX = (atlasNode->position - posY*inState->atlasWidth) * GLC_TEXTURE_SIZE;
    posY *= GLC_TEXTURE_SIZE;

    outline.flags |= FT_OUTLINE_HIGH_PRECISION;
  }
  else {
    pixmap.width = __glcNextPowerOf2(
            (boundingBox.xMax - boundingBox.xMin + 63) >> 6); /* ceil() */
    pixmap.rows = __glcNextPowerOf2(
            (boundingBox.yMax - boundingBox.yMin + 63) >> 6); /* ceil() */
    if (!__glcTextureGetImmediate(inState, pixmap.width, pixmap.rows))
      return;

    texture = inState->texture.id;
    texWidth = inState->texture.width;
    texHeigth = inState->texture.heigth;
    texScaleX = scale_x;
    texScaleY = scale_y;
    posX = 0;
    posY = 0;
  }

  pixmap.pitch = pixmap.width; /* 8 bits / pixel */

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_grays; /* Anti-aliased rendering */
  pixmap.num_grays = 256;
  pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
  if (!pixmap.buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Flip the picture */
  pixmap.pitch = - pixmap.pitch;

  /* Create the texture */
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  matrix.xx = 32768; /* 0.5 in FT_Fixed type */
  matrix.yy = 32768;

  /* Make a copy of the bounding box for use when creating mipmaps */
  mipmapBoundingBox.xMin = boundingBox.xMin;
  mipmapBoundingBox.yMin = boundingBox.yMin;

  /* Iterate on the powers of 2 in order to build the mipmap */
  while ((pixmap.width > minSize) && (pixmap.rows > minSize)) {
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, - pixmap.rows * pixmap.pitch);

    /* translate the outline to match (0,0) with the glyph's lower left
     * corner
     */
    FT_Outline_Translate(&outline, -mipmapBoundingBox.xMin,
			 -mipmapBoundingBox.yMin);

    /* render the glyph */
    if (FT_Outline_Get_Bitmap(inState->library, &outline, &pixmap)) {
      glPopClientAttrib();
      __glcFree(pixmap.buffer);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    glTexSubImage2D(GL_TEXTURE_2D, level, posX >> level, posY >> level,
		    pixmap.width, pixmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE,
		    pixmap.buffer);

    /* A mipmap is built only if a display list is currently building
     * otherwise it adds useless computations
     */
    if (!(inState->enableState.mipmap && inDisplayListIsBuilding))
      break;

    /* restore the outline initial position */
    FT_Outline_Translate(&outline, mipmapBoundingBox.xMin,
			 mipmapBoundingBox.yMin);

    level++; /* Next level of mipmap */
    pixmap.width >>= 1;
    pixmap.rows >>= 1;
    pixmap.pitch >>= 1;

    /* For the next level of mipmap, the character size must divided by 2. */
    FT_Outline_Transform(&outline, &matrix);

    /* Get the bounding box of the transformed glyph */
    FT_Outline_Get_CBox(&outline, &mipmapBoundingBox);
  }

  /* Finish to build the mipmap if necessary */
  if (inState->enableState.mipmap && inDisplayListIsBuilding) {
    if (GLEW_VERSION_1_2 || GLEW_SGIS_texture_lod)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);
    else {
      /* The OpenGL driver does not support the extension GL_EXT_texture_lod 
       * We must finish the pixmap until the mipmap level is 1x1.
       * Here the smaller mipmap levels will be transparent, no glyph will be
       * rendered.
       * TODO: Use gluScaleImage() to render the last levels.
       */
      memset(pixmap.buffer, 0, pixmap.width * pixmap.rows);
      while ((pixmap.width > 0) || (pixmap.rows > 0)) {
	glTexSubImage2D(GL_TEXTURE_2D, level, posX >> level, posY >> level,
		     pixmap.width ? pixmap.width : 1,
		     pixmap.rows ? pixmap.rows : 1, GL_ALPHA,
		     GL_UNSIGNED_BYTE, pixmap.buffer);

	level++;
	pixmap.width >>= 1;
	pixmap.rows >>= 1;
      }
    }
  }

  glPopClientAttrib();

  /* Add the new texture to the texture list and the new display list
   * to the list of display lists
   */
  if (inState->enableState.glObjects) {
    inGlyph->displayList[0] = glGenLists(1);
    if (!inGlyph->displayList[0]) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(pixmap.buffer);
      return;
    }

    /* Create the display list */
    glNewList(inGlyph->displayList[0], GL_COMPILE_AND_EXECUTE);
  }

  /* Compute the size of the glyph */
  width = (GLfloat)((boundingBox.xMax - boundingBox.xMin) / 64.);
  heigth = (GLfloat)((boundingBox.yMax - boundingBox.yMin) / 64.);

  /* Do the actual GL rendering */
  glBegin(GL_QUADS);
  glNormal3f(0., 0., 1.);
  glTexCoord2f(posX / texWidth, posY / texHeigth);
  glVertex2f(boundingBox.xMin / 64. / texScaleX, 
	     boundingBox.yMin / 64. / texScaleY);
  glTexCoord2f((posX + width) / texWidth, posY / texHeigth);
  glVertex2f(boundingBox.xMax / 64. / texScaleX,
	     boundingBox.yMin / 64. / texScaleY);
  glTexCoord2f((posX + width) / texWidth, (posY + heigth) / texHeigth);
  glVertex2f(boundingBox.xMax / 64. / texScaleX,
	     boundingBox.yMax / 64. / texScaleY);
  glTexCoord2f(posX / texWidth, (posY + heigth) / texHeigth);
  glVertex2f(boundingBox.xMin / 64. / texScaleX,
	     boundingBox.yMax / 64. / texScaleY);
  glEnd();

  /* Store the glyph advance in the display list */
  glTranslatef(face->glyph->advance.x / 64. / scale_x,
	       face->glyph->advance.y / 64. / scale_y, 0.);

  if (inState->enableState.glObjects) {
    /* Finish display list creation */
    glEndList();
  }

  __glcFree(pixmap.buffer);
}
