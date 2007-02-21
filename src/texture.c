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

/** \file
 *  defines the routines used to render characters with textures.
 */

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



/* This function is called when a glyph is destroyed, so that it is removed from
 * the atlas list if relevant.
 */
void __glcDeleteAtlasElement(__GLCatlasElement* This,
			     __GLCcontext* inContext)
{
  FT_List_Remove(&inContext->atlasList, (FT_ListNode)This);
  inContext->atlasCount--;
  free(This);
}



/* This function get some room in the texture atlas for a new glyph 'inGlyph'.
 * Eventually it creates the texture atlas, if it does not exist yet.
 */
static GLboolean __glcTextureAtlasGetPosition(__GLCcontext* inContext,
					      __GLCglyph* inGlyph)
{
  __GLCatlasElement* atlasNode = NULL;

  /* Test if the atlas already exists. If not, create it. */
  if (!inContext->atlas.id) {
    int size = 1024; /* Initial try with a 1024x1024 texture */
    int i = 0;
    GLint format = 0;
    GLint level = 0;

    /* Not all gfx card are able to use 1024x1024 textures (especially old ones
     * like 3dfx's). Moreover, the texture memory may be scarce when our texture
     * will be created, so we try several texture sizes : first 1024x1024 then
     * if it fails, we try 512x512 then 256x256. All gfx cards support 256x256
     * textures so if it fails with this texture size, that is because we ran
     * out of texture memory. In such a case, there is nothing we can do, so the
     * routine aborts with GLC_RESOURCE_ERROR raised.
     */
    for (i = 0; i < 3; i++) {
      glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, size,
		   size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			       &format);
      if (format)
        break;

      size >>= 1;
    }

    /* Out of texture memory : abortion */
    if (i == 3) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GL_FALSE;
    }

    /* Create the texture atlas structure. The texture is divided in small
     * square areas of GLC_TEXTURE_SIZE x GLC_TEXTURE_SIZE, each of which will
     * contain a different glyph.
     * TODO: Allow the user to change GLC_TEXTURE_SIZE rather than using a fixed
     * value.
     */
    glGenTextures(1, &inContext->atlas.id);
    inContext->atlas.width = size;
    inContext->atlas.heigth = size;
    inContext->atlasWidth = size / GLC_TEXTURE_SIZE;
    inContext->atlasHeight = size / GLC_TEXTURE_SIZE;
    inContext->atlasCount = 0;
    glBindTexture(GL_TEXTURE_2D, inContext->atlas.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, size,
		 size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);

    /* Create the mipmap structure of the texture atlas, no matter if GLC_MIPMAP
     * is enabled or not.
     */
    while (size > 1) {
      size >>= 1;
      level++;
      glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA8, size,
		   size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
    }

    /* Use trilinear filtering if GLC_MIPMAP is enabled.
     * Otherwise use bilinear filtering.
     */
    if (inContext->enableState.mipmap)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		      GL_LINEAR_MIPMAP_LINEAR);
    else
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		      GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  }

  /* At this stage, we want to get a free area in the texture atlas in order to
   * store a new glyph. Two situations may occur : the atlas is full or not
   */
  if (inContext->atlasCount == inContext->atlasWidth * inContext->atlasHeight) {
    /* The texture atlas is full. We must release an area to re-use it.
     * We get the glyph that has not been used for a long time (that is the
     * tail element of atlasList).
     */
    atlasNode = (__GLCatlasElement*)inContext->atlasList.tail;
    assert(atlasNode);
    /* Release the texture area of the glyph */
    __glcGlyphDestroyTexture(atlasNode->glyph);
    /* Put the texture area at the head of the list otherwise we will use the
     * same texture element over and over again each time that we need to
     * release a texture area.
     */
    FT_List_Up(&inContext->atlasList, (FT_ListNode)atlasNode);
  }
  else {
    /* The texture atlas is not full. We create a new texture area and we store
     * its definition in atlas list.
     */
    atlasNode = (__GLCatlasElement*)__glcMalloc(sizeof(__GLCatlasElement));
    if (!atlasNode) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GL_FALSE;
    }

    atlasNode->node.data = atlasNode;
    atlasNode->position = inContext->atlasCount++;
    FT_List_Insert(&inContext->atlasList, (FT_ListNode)atlasNode);
  }

  /* Update the texture element */
  atlasNode->glyph = inGlyph;
  inGlyph->textureObject = atlasNode;

  return GL_TRUE;
}



/* For immediate rendering mode (that is when GLC_GL_OBJECTS is disabled), this
 * function returns a texture that will store the glyph that is intended to be
 * rendered. If the texture does not exist yet, it is created.
 */
static GLboolean __glcTextureGetImmediate(__GLCcontext* inContext,
					  GLsizei inWidth, GLsizei inHeight)
{
  GLint format = 0;

  /* Check if a texture exists to store the glyph */
  if (inContext->texture.id) {
    /* Check if the texture size is large enough to store the glyph */
    if ((inWidth > inContext->texture.width)
	|| (inHeight > inContext->texture.heigth)) {
      /* The texture is not large enough so we destroy the current texture */
      glDeleteTextures(1, &inContext->texture.id);
      inContext->texture.id = 0;
      inContext->texture.width = 0;
      inContext->texture.heigth = 0;
    }
    else {
      /* The texture is large enough, it is already bound to the current
       * GL context.
       */
      return GL_TRUE;
    }
  }

  if (GLEW_ARB_pixel_buffer_object)
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

  /* Check if a new texture can be created */
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, inWidth,
	       inHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			   &format);
  /* TODO: If the texture creation fails, try with a smaller size */
  if (!format) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  /* Create a texture object and make it current */
  glGenTextures(1, &inContext->texture.id);
  glBindTexture(GL_TEXTURE_2D, inContext->texture.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, inWidth,
	       inHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  /* For immediate mode rendering, always use bilinear filtering even if
   * GLC_MIPMAP is enabled : we have determined the size of the glyph when it
   * will be rendered on the screen and the texture size has been defined
   * accordingly. Hence the mipmap levels would not be used, so there is no
   * point in spending time to compute them.
   */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  inContext->texture.width = inWidth;
  inContext->texture.heigth = inHeight;

  if (GLEW_ARB_pixel_buffer_object) {
    /* Create a PBO, if none exists yet */
    if (!inContext->texture.bufferObjectID) {
      glGenBuffersARB(1, &inContext->texture.bufferObjectID);
      if (!inContext->texture.bufferObjectID) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	/* Even though we failed to create a PBO ID, the rendering of the glyph
	 * can be processed without PBO, so we return GL_TRUE.
	 */
	return GL_TRUE;
      }
    }
    /* Bind the buffer and define/update its size */
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
		    inContext->texture.bufferObjectID);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, inWidth * inHeight, NULL,
		    GL_STREAM_DRAW_ARB);
  }

  return GL_TRUE;
}



/* Internal function that renders glyph in textures :
 * 'inCode' must be given in UCS-4 format
 */
void __glcRenderCharTexture(__GLCfont* inFont,
			    __GLCcontext* inContext,
			    GLboolean inDisplayListIsBuilding,
			    GLfloat scale_x, GLfloat scale_y,
			    __GLCglyph* inGlyph)
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

#ifndef FT_CACHE_H
  face = inFont->faceDesc->face;
  assert(face);
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)inFont->faceDesc,
			     &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
#endif

  outline = face->glyph->outline;

  if (inDisplayListIsBuilding) {
    if (!__glcTextureAtlasGetPosition(inContext, inGlyph))
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
    __GLCatlasElement* atlasNode = (__GLCatlasElement*)inGlyph->textureObject;

    pixmap.width = GLC_TEXTURE_SIZE;
    pixmap.rows = GLC_TEXTURE_SIZE;
    texture = inContext->atlas.id;
    texWidth = inContext->atlas.width;
    texHeigth = inContext->atlas.heigth;
    posY = (atlasNode->position / inContext->atlasWidth);
    posX = (atlasNode->position - posY*inContext->atlasWidth)*GLC_TEXTURE_SIZE;
    posY *= GLC_TEXTURE_SIZE;

    outline.flags |= FT_OUTLINE_HIGH_PRECISION;
  }
  else {
    pixmap.width = __glcNextPowerOf2(
            (boundingBox.xMax - boundingBox.xMin + 63) >> 6); /* ceil() */
    pixmap.rows = __glcNextPowerOf2(
            (boundingBox.yMax - boundingBox.yMin + 63) >> 6); /* ceil() */
    if (!__glcTextureGetImmediate(inContext, pixmap.width, pixmap.rows))
      return;

    texture = inContext->texture.id;
    texWidth = inContext->texture.width;
    texHeigth = inContext->texture.heigth;
    posX = 0;
    posY = 0;
  }

  pixmap.pitch = pixmap.width; /* 8 bits / pixel */

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_grays; /* Anti-aliased rendering */
  pixmap.num_grays = 256;

  if (!inContext->texture.bufferObjectID || inDisplayListIsBuilding) {
    pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
    if (!pixmap.buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
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
    if (GLEW_ARB_pixel_buffer_object && !inDisplayListIsBuilding) {
      pixmap.buffer = (GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
						GL_WRITE_ONLY);
      if (!pixmap.buffer) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, - pixmap.rows * pixmap.pitch);

    /* translate the outline to match (0,0) with the glyph's lower left
     * corner
     */
    FT_Outline_Translate(&outline, -mipmapBoundingBox.xMin,
			 -mipmapBoundingBox.yMin);

    /* render the glyph */
    if (FT_Outline_Get_Bitmap(inContext->library, &outline, &pixmap)) {
      glPopClientAttrib();
      __glcFree(pixmap.buffer);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    if (GLEW_ARB_pixel_buffer_object && !inDisplayListIsBuilding) {
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      pixmap.buffer = NULL;
    }

    glTexSubImage2D(GL_TEXTURE_2D, level, posX >> level, posY >> level,
		    pixmap.width, pixmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE,
		    pixmap.buffer);

    /* A mipmap is built only if a display list is currently building
     * otherwise it adds useless computations
     */
    if (!(inContext->enableState.mipmap && inDisplayListIsBuilding))
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
  if (inContext->enableState.mipmap && inDisplayListIsBuilding) {
    if (GLEW_VERSION_1_2 || GLEW_SGIS_texture_lod)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);
    else {
      /* The OpenGL driver does not support the extension GL_EXT_texture_lod 
       * We must finish the pixmap until the mipmap level is 1x1.
       * Here the smaller mipmap levels will be transparent, no glyph will be
       * rendered.
       * TODO: Use gluScaleImage() to render the last levels.
       * Here we do not take the GL_ARB_pixel_buffer_object into account
       * because there are few chances that a gfx card that supports PBO, does
       * not support texture levels.
       */
      assert(!GLEW_ARB_pixel_buffer_object);
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

  /* Compute the size of the glyph */
  width = (GLfloat)((boundingBox.xMax - boundingBox.xMin) / 64.);
  heigth = (GLfloat)((boundingBox.yMax - boundingBox.yMin) / 64.);

  /* Add the new texture to the texture list and the new display list
   * to the list of display lists
   */
  if (inContext->enableState.glObjects) {
    inGlyph->displayList[0] = glGenLists(1);
    if (!inGlyph->displayList[0]) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(pixmap.buffer);
      return;
    }

    /* Create the display list */
    glNewList(inGlyph->displayList[0], GL_COMPILE_AND_EXECUTE);
    glScalef(1. / 64. / scale_x, 1. / 64. / scale_y , 1.);

    /* Modify the bouding box dimensions to compensate the glScalef() */
    boundingBox.xMin *= scale_x / GLC_TEXTURE_SIZE;
    boundingBox.xMax *= scale_x / GLC_TEXTURE_SIZE;
    boundingBox.yMin *= scale_y / GLC_TEXTURE_SIZE;
    boundingBox.yMax *= scale_y / GLC_TEXTURE_SIZE;
  }

  /* Do the actual GL rendering */
  glBegin(GL_QUADS);
  glNormal3f(0., 0., 1.);
  glTexCoord2f(posX / texWidth, posY / texHeigth);
  glVertex2i(boundingBox.xMin, boundingBox.yMin);
  glTexCoord2f((posX + width) / texWidth, posY / texHeigth);
  glVertex2i(boundingBox.xMax, boundingBox.yMin);
  glTexCoord2f((posX + width) / texWidth, (posY + heigth) / texHeigth);
  glVertex2i(boundingBox.xMax, boundingBox.yMax);
  glTexCoord2f(posX / texWidth, (posY + heigth) / texHeigth);
  glVertex2i(boundingBox.xMin, boundingBox.yMax);
  glEnd();

  /* Store the glyph advance in the display list */
  glTranslatef(face->glyph->advance.x, face->glyph->advance.y, 0.);

  if (inContext->enableState.glObjects) {
    /* Finish display list creation */
    glScalef(64. * scale_x, 64. * scale_y, 1.);
    glEndList();
  }

  __glcFree(pixmap.buffer);
}
