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
 *  defines the routines used to render characters with textures.
 */

#include "internal.h"

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "texture.h"



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
    inContext->atlas.height = size;
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
    __glcGlyphDestroyTexture(atlasNode->glyph, inContext);
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

  if (GLEW_ARB_vertex_buffer_object) {
    /* Create a VBO, if none exists yet */
    if (!inContext->atlas.bufferObjectID) {
      glGenBuffersARB(1, &inContext->atlas.bufferObjectID);
      if (!inContext->atlas.bufferObjectID) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	/* Even though we failed to create a VBO ID, the rendering of the glyph
	 * can be processed without VBO, so we return GL_TRUE.
	 */
	return GL_TRUE;
      }
    }
    /* Bind the buffer and define/update its size */
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, inContext->atlas.bufferObjectID);
  }

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
	|| (inHeight > inContext->texture.height)) {
      /* The texture is not large enough so we destroy the current texture */
      glDeleteTextures(1, &inContext->texture.id);
      inWidth = (inWidth > inContext->texture.width) ?
	inWidth : inContext->texture.width;
      inHeight = (inHeight > inContext->texture.height) ?
	inHeight : inContext->texture.height;
      inContext->texture.id = 0;
      inContext->texture.width = 0;
      inContext->texture.height = 0;
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
  inContext->texture.height = inHeight;

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
void __glcRenderCharTexture(__GLCfont* inFont, __GLCcontext* inContext,
			    GLfloat scale_x, GLfloat scale_y,
			    __GLCglyph* inGlyph)
{
  GLfloat width = 0, height = 0;
  GLint level = 0;
  GLint posX = 0, posY = 0;
  GLint pixWidth = 0, pixHeight = 0;
  void* pixBuffer = NULL;
  GLint boundingBox[4] = {0, 0, 0, 0};
  int minSize = (GLEW_VERSION_1_2 || GLEW_SGIS_texture_lod) ? 2 : 1;
  GLfloat texWidth = 0, texHeight = 0;

  if (inContext->enableState.glObjects) {
    __GLCatlasElement* atlasNode = NULL;

    if (!__glcTextureAtlasGetPosition(inContext, inGlyph))
      return;

    /* Compute the size of the pixmap where the glyph will be rendered */
    atlasNode = inGlyph->textureObject;

    __glcFaceDescGetBitmapSize(inFont->faceDesc, &pixWidth, &pixHeight,
                               boundingBox, scale_x, scale_y, 0, inContext);

    texWidth = inContext->atlas.width;
    texHeight = inContext->atlas.height;
    posY = (atlasNode->position / inContext->atlasWidth);
    posX = (atlasNode->position - posY*inContext->atlasWidth)*GLC_TEXTURE_SIZE;
    posY *= GLC_TEXTURE_SIZE;
  }
  else {
    int factor = 0;

    /* Try several texture size until we are able to create one */
    do {
      if (!__glcFaceDescGetBitmapSize(inFont->faceDesc, &pixWidth, &pixHeight,
                                      boundingBox, scale_x, scale_y, factor,
                                      inContext))
	return;

      factor = 1;
    } while (!__glcTextureGetImmediate(inContext, pixWidth, pixHeight));

    texWidth = inContext->texture.width;
    texHeight = inContext->texture.height;
    posX = 0;
    posY = 0;
  }

  if (!inContext->texture.bufferObjectID || inContext->enableState.glObjects) {
    pixBuffer = (GLubyte *)__glcMalloc(pixWidth * pixHeight);
    if (!pixBuffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }

  /* Create the texture */
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /* Iterate on the powers of 2 in order to build the mipmap */
  do {
    if (GLEW_ARB_pixel_buffer_object && !inContext->enableState.glObjects) {
      pixBuffer = (GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
						GL_WRITE_ONLY_ARB);
      if (!pixBuffer) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }

    /* render the glyph */
    if (!__glcFaceDescGetBitmap(inFont->faceDesc, pixWidth, pixHeight,
                                pixBuffer, inContext)) {
      glPopClientAttrib();

      if (GLEW_ARB_pixel_buffer_object && !inContext->enableState.glObjects)
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      else
        __glcFree(pixBuffer);

      return;
    }

    if (GLEW_ARB_pixel_buffer_object && !inContext->enableState.glObjects) {
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      pixBuffer = NULL;
    }

    glTexSubImage2D(GL_TEXTURE_2D, level, posX >> level, posY >> level,
		    pixWidth, pixHeight, GL_ALPHA, GL_UNSIGNED_BYTE,
		    pixBuffer);

    /* A mipmap is built only if a display list is currently building
     * otherwise it adds useless computations
     */
    if (!(inContext->enableState.mipmap && inContext->enableState.glObjects))
      break;

    level++; /* Next level of mipmap */
    pixWidth >>= 1;
    pixHeight >>= 1;
  } while ((pixWidth > minSize) && (pixHeight > minSize));

  /* Finish to build the mipmap if necessary */
  if (inContext->enableState.mipmap && inContext->enableState.glObjects) {
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
      memset(pixBuffer, 0, pixWidth * pixHeight);
      while ((pixWidth > 0) || (pixHeight > 0)) {
	glTexSubImage2D(GL_TEXTURE_2D, level, posX >> level, posY >> level,
		     pixWidth ? pixWidth : 1,
		     pixHeight ? pixHeight : 1, GL_ALPHA,
		     GL_UNSIGNED_BYTE, pixBuffer);

	level++;
	pixWidth >>= 1;
	pixHeight >>= 1;
      }
    }
  }

  glPopClientAttrib();

  /* Compute the size of the glyph */
  width = (GLfloat)((boundingBox[2] - boundingBox[0]) / 64.)
    + GLC_TEXTURE_PADDING;
  height = (GLfloat)((boundingBox[3] - boundingBox[1]) / 64.)
    + GLC_TEXTURE_PADDING;

  if (pixBuffer)
    __glcFree(pixBuffer);

  /* Add the new texture to the texture list and the new display list
   * to the list of display lists
   */
  if (inContext->enableState.glObjects) {
    if (GLEW_ARB_vertex_buffer_object) {
      GLfloat* buffer = NULL;
      GLfloat* data = NULL;
      __GLCatlasElement* atlasNode = inGlyph->textureObject;

      buffer = (GLfloat*)__glcMalloc(inContext->atlasWidth
				     * inContext->atlasHeight * 20
				     * sizeof(GLfloat));
      if (!buffer) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }

      /* The display list ID is used as a flag to declare that the VBO has been
       * initialized and can be used.
       */
      inGlyph->glObject[1] = 0xffffffff;

      /* Here we do not use the GL command glBufferSubData() since it seems to
       * be buggy on some GL drivers (the DRI Intel specifically).
       * Instead, we use a workaround: the current values of the VBO are stored
       * in memory and new values are appended to them. Then, the content of
       * the resulting array replaces all the values previously stored in the
       * VBO.
       */
      if (inContext->atlasCount > 1) {
	data = (GLfloat*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_ONLY);
	if (!data) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  __glcFree(buffer);
	  return;
	}
	memcpy(buffer, data, inContext->atlasCount * 20 * sizeof(GLfloat));
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
      }

      data = buffer + atlasNode->position * 20;

      data[0] = posX / texWidth;
      data[1] = posY / texHeight;
      data[2] = (boundingBox[0] / 64. - (GLC_TEXTURE_PADDING >> 1))
	/ (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      data[3] = (boundingBox[1] / 64. - (GLC_TEXTURE_PADDING >> 1))
	/ (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      data[4] = 0.f;
      data[5] = (posX + width) / texWidth;
      data[6] = data[1];
      data[7] = (boundingBox[2] / 64. + (GLC_TEXTURE_PADDING >> 1))
	/ (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      data[8] = data[3];
      data[9] = 0.f;
      data[10] = data[5];
      data[11] = (posY + height) / texHeight;
      data[12] = data[7];
      data[13] = (boundingBox[3] / 64. + (GLC_TEXTURE_PADDING >> 1))
	/ (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      data[14] = 0.f;
      data[15] = data[0];
      data[16] = data[11];
      data[17] = data[2];
      data[18] = data[13];
      data[19] = 0.f;

      /* Size of the buffer data is equal to the number of glyphes than can be
       * stored in the texture times 20 GLfloat (4 vertices made of 3D
       * coordinates plus 2D texture coordinates : 4 * (3 + 2) = 20)
       */
      glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		      inContext->atlasWidth * inContext->atlasHeight
		      * 20 * sizeof(GLfloat), buffer, GL_STATIC_DRAW_ARB);

      __glcFree(buffer);

      /* Do the actual GL rendering */
      glInterleavedArrays(GL_T2F_V3F, 0, NULL);
      glNormal3f(0.f, 0.f, 1.f);
      glDrawArrays(GL_QUADS, atlasNode->position * 4, 4);

      return;
    }
    else {
      inGlyph->glObject[1] = glGenLists(1);
      if (!inGlyph->glObject[1]) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }

      /* Create the display list */
      glNewList(inGlyph->glObject[1], GL_COMPILE);
      glScalef(1. / 64. / scale_x, 1. / 64. / scale_y , 1.);

      /* Modify the bouding box dimensions to compensate the glScalef() */
      boundingBox[0] *= scale_x / (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      boundingBox[1] *= scale_y / (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      boundingBox[2] *= scale_x / (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
      boundingBox[3] *= scale_y / (GLC_TEXTURE_SIZE - GLC_TEXTURE_PADDING);
    }
  }

  /* Do the actual GL rendering */
  glBegin(GL_QUADS);
  glNormal3f(0.f, 0.f, 1.f);
  glTexCoord2f(posX / texWidth, posY / texHeight);
  glVertex2i(boundingBox[0] - (GLC_TEXTURE_PADDING << 5),
	     boundingBox[1] - (GLC_TEXTURE_PADDING << 5));
  glTexCoord2f((posX + width) / texWidth, posY / texHeight);
  glVertex2i(boundingBox[2] + (GLC_TEXTURE_PADDING << 5),
	     boundingBox[1] - (GLC_TEXTURE_PADDING << 5));
  glTexCoord2f((posX + width) / texWidth, (posY + height) / texHeight);
  glVertex2i(boundingBox[2] + (GLC_TEXTURE_PADDING << 5),
	     boundingBox[3] + (GLC_TEXTURE_PADDING << 5));
  glTexCoord2f(posX / texWidth, (posY + height) / texHeight);
  glVertex2i(boundingBox[0] - (GLC_TEXTURE_PADDING << 5),
	     boundingBox[3] + (GLC_TEXTURE_PADDING << 5));
  glEnd();

  if (inContext->enableState.glObjects) {
    /* Finish display list creation */
    glScalef(64. * scale_x, 64. * scale_y, 1.);
    glEndList();
    glCallList(inGlyph->glObject[1]);
  }
}
