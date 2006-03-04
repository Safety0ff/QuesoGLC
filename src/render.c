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

/** \file
 * defines the so-called "Rendering commands" described in chapter 3.9 of the
 * GLC specs.
 */

/** \defgroup render Rendering commands
 * These are the commands that render characters to a GL render target. Those
 * commands gather glyph datas according to the parameters that has been set in
 * the state machine of GLC, and issue GL commands to render the characters
 * layout to the GL render target.
 *
 * When it renders a character, GLC finds a font that maps the character code
 * to a character such as LATIN CAPITAL LETTER A, then uses one or more glyphs
 * from the font to create a graphical layout that represents the character.
 * Finally, GLC issues a sequence of GL commands to draw the layout. Glyph
 * coordinates are defined in EM units and are transformed during rendering to
 * produce the desired mapping of the glyph shape into the GL window coordinate
 * system.
 *
 * If GLC cannot find a font that maps the character code in the list
 * \b GLC_CURRENT_FONT_LIST, it attemps to produce an alternate rendering. If
 * the value of the boolean variable \b GLC_AUTO_FONT is set to \b GL_TRUE, GLC
 * searches for a font that has the character that maps the character code. If
 * the search succeeds, the font's ID is appended to \b GLC_CURRENT_FONT_LIST
 * and the character is rendered.
 *
 * If there are fonts in the list \b GLC_CURRENT_FONT_LIST, but a match for
 * the character code cannot be found in any of those fonts, GLC goes through
 * these steps :
 * -# If the value of the variable \b GLC_REPLACEMENT_CODE is nonzero,
 * GLC finds a font that maps the replacement code, and renders the character
 * that the replacement code is mapped to.
 * -# If the variable \b GLC_REPLACEMENT_CODE is zero, or if the replacement
 * code does not result in a match, GLC checks whether a callback function is
 * defined. If a callback function is defined for \b GLC_OP_glcUnmappedCode, GLC
 * calls the function. The callback function provides the character code to the
 * user and allows loading of the appropriate font. After the callback returns,
 * GLC tries to render the character code again.
 * -# Otherwise, the command attemps to render the character sequence
 * <em>\\\<hexcode\></em>, where \\ is the character REVERSE SOLIDUS (U+5C),
 * \< is the character LESS-THAN SIGN (U+3C), \> is the character GREATER-THAN
 * SIGN (U+3E), and \e hexcode is the character code represented as a sequence
 * of hexadecimal digits. The sequence has no leading zeros, and alphabetic
 * digits are in upper case. The GLC measurement commands treat the sequence
 * as a single character.
 *
 * The rendering commands raise \b GLC_PARAMETER_ERROR if the callback function
 * defined for \b GLC_OP_glcUnmappedCode is called and the current string type
 * is \b GLC_UTF8_QSO.
 *
 * \note Some rendering commands create and/or use display lists and/or
 * textures. The IDs of those display lists and textures are stored in the
 * current GLC context but the display lists and the textures themselves are
 * managed by the current GL context. In order not to impact the performance of
 * error-free programs, QuesoGLC does not check if the current GL context is
 * the same than the one where the display lists and the textures were actually
 * created. If the current GL context has changed meanwhile, the result of
 * commands that refer to the corresponding display lists or textures is
 * undefined.
 *
 * As a reminder, the render commands may issue GL commands, hence a GL context
 * must be bound to the current thread such that the GLC commands produce the
 * desired result. It is the responsibility of the GLC client to set up the
 * underlying GL implementation.
 */

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <math.h>

#include "internal.h"
#include FT_OUTLINE_H
#include FT_LIST_H

/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(__glcFont* inFont,
				  __glcContextState* inState,   FT_UInt glyphIndex)
{
  FT_Matrix matrix;
  FT_Face face = __glcFaceDescOpen(inFont->faceDesc, inState);
  FT_Outline outline;
  FT_BBox boundBox;
  FT_Bitmap pixmap;
  GLfloat *transform = inState->bitmapMatrix;
  GLfloat determinant = 0., norm = 0.;
  GLfloat scale_x = 0., scale_y = 0.;
  int i = 0;

  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  outline = face->glyph->outline;

  /* Compute the norm of the transformation matrix */
  for (i = 0; i < 4; i++) {
    if (abs(transform[i]) > norm)
      norm = abs(transform[i]);
  }

  determinant = transform[0] * transform[3] - transform[1] * transform[2];

  /* If the transformation is degenerated, nothing needs to be rendered */
  if (abs(determinant) < norm * GLC_EPSILON) {
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  scale_x = sqrt(transform[0]*transform[0]+transform[1]*transform[1]);
  scale_y = sqrt(transform[2]*transform[2]+transform[3]*transform[3]);

  if (FT_Set_Char_Size(face, (FT_F26Dot6)(scale_x * 64.), (FT_F26Dot6)(scale_y * 64.),
		       inState->resolution, inState->resolution)) {
    __glcFaceDescClose(inFont->faceDesc);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Get and load the glyph which unicode code is identified by inCode */
  if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP |
		    FT_LOAD_IGNORE_TRANSFORM)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* compute glyph dimensions */
  matrix.xx = (FT_Fixed)(transform[0] * 65536. / scale_x);
  matrix.xy = (FT_Fixed)(transform[2] * 65536. / scale_y);
  matrix.yx = (FT_Fixed)(transform[1] * 65536. / scale_x);
  matrix.yy = (FT_Fixed)(transform[3] * 65536. / scale_y);

  /* Get the bounding box of the glyph */
  FT_Outline_Transform(&outline, &matrix);
  FT_Outline_Get_CBox(&outline, &boundBox);

  /* Compute the sizes of the pixmap where the glyph will be drawn */
  boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
  boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
  boundBox.xMax = (boundBox.xMax + 63) & -64;	/* ceiling(xMax) */
  boundBox.yMax = (boundBox.yMax + 63) & -64;	/* ceiling(yMax) */

  pixmap.width = (boundBox.xMax - boundBox.xMin) >> 6;
  pixmap.rows = (boundBox.yMax - boundBox.yMin) >> 6;
  pixmap.pitch = ((pixmap.width + 4) >> 3) + 1;	/* 1 bit / pixel */
  pixmap.width = pixmap.pitch << 3;

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
  pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
  if (!pixmap.buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* fill the pixmap buffer with the background color */
  memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);

  /* Flip the picture */
  pixmap.pitch = - pixmap.pitch;

  /* translate the outline to match (0,0) with the glyph's lower left corner */
  FT_Outline_Translate(&outline, -((boundBox.xMin + 32) & -64),
		       -((boundBox.yMin + 32) & -64));

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inState->library, &outline, &pixmap)) {
    __glcFree(pixmap.buffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* Do the actual GL rendering */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBitmap(pixmap.width, pixmap.rows, -boundBox.xMin >> 6, -boundBox.yMin >> 6,
	   face->glyph->advance.x / 64. * matrix.xx / 65536.
	   + face->glyph->advance.y / 64. * matrix.xy / 65536.,
	   face->glyph->advance.x / 64. * matrix.yx / 65536.
	   + face->glyph->advance.y / 64. * matrix.yy / 65536.,
	   pixmap.buffer);

  __glcFree(pixmap.buffer);

  if (FT_Set_Char_Size(face, GLC_POINT_SIZE << 6, GLC_POINT_SIZE << 6,
		       inState->resolution, inState->resolution)) {
    __glcFaceDescClose(inFont->faceDesc);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  __glcFaceDescClose(inFont->faceDesc);
}


static int __glcNextPowerOf2(int value)
{
  int power = 0;

  for (power = 1; power < value; power <<= 1);

  return power;
}

/* Internal function that renders glyph in textures :
 * 'inCode' must be given in UCS-4 format
 */
static void __glcRenderCharTexture(__glcFont* inFont,
				   __glcContextState* inState, GLint inCode,
				   GLboolean inDisplayListIsBuilding)
{
  FT_Face face = __glcFaceDescOpen(inFont->faceDesc, inState);
  FT_Outline outline;
  FT_BBox boundBox;
  FT_Bitmap pixmap;
  GLuint texture = 0;
  GLfloat width = 0, height = 0;
  GLint format = 0;
  __glcDisplayListKey *dlKey = NULL;
  FT_ListNode node = NULL;
  GLint boundTexture = 0;
  GLint unpackAlignment = 0;

  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  outline = face->glyph->outline;

  /* Get the bounding box of the glyph */
  FT_Outline_Get_CBox(&outline, &boundBox);

  /* Compute the size of the pixmap where the glyph will be rendered */
  boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
  boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
  boundBox.xMax = (boundBox.xMax + 63) & -64;	/* ceiling(xMax) */
  boundBox.yMax = (boundBox.yMax + 63) & -64;	/* ceiling(yMax) */

  pixmap.width = __glcNextPowerOf2((boundBox.xMax - boundBox.xMin) >> 6);
  pixmap.rows = __glcNextPowerOf2((boundBox.yMax - boundBox.yMin) >> 6);
  pixmap.pitch = pixmap.width;		/* 8 bits / pixel */

  /* Check if a new texture can be created */
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, pixmap.width,
	       pixmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			   &format);
  if (!format) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_grays;	/* Anti-aliased rendering */
  pixmap.num_grays = 256;
  pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
  if (!pixmap.buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* fill the pixmap buffer with the background color */
  memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);

  /* Flip the picture */
  pixmap.pitch = - pixmap.pitch;

  /* translate the outline to match (0,0) with the glyph's lower left corner */
  FT_Outline_Translate(&outline, -((boundBox.xMin + 32) & -64),
		       -((boundBox.yMin + 32) & -64));

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inState->library, &outline, &pixmap)) {
    __glcFree(pixmap.buffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(inFont->faceDesc);
    return;
  }

  /* Create a texture object and make it current */
  glGenTextures(1, &texture);

  /* Add the new texture to the texture list */
  if (inState->glObjects) {
    FT_ListNode node = NULL;

    node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
    if (node) {
      /* Hack in order to be able to store a 32 bits texture ID in a 32/64 bits
       * void* pointer so that we do not need to allocate memory just to store
       * a single integer value
       */
      union {
	void* ptr;
	GLuint ui;
      } voidToGLuint;

      voidToGLuint.ui = texture;

      node->data = voidToGLuint.ptr;
      FT_List_Add(&inFont->parent->textureObjectList, node);
    }
    else {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      glDeleteTextures(1, &texture);
      __glcFree(pixmap.buffer);
      __glcFaceDescClose(inFont->faceDesc);
      return;
    }
  }

  /* Create the texture */
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, texture);

  /* A mipmap is built only if a display list is currently building
   * otherwise it adds uneeded computations
   */
  if (inState->mipmap && inDisplayListIsBuilding) {
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA8, pixmap.width,
		      pixmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE,
		      pixmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		    GL_LINEAR_MIPMAP_LINEAR);
  }
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, pixmap.width,
		 pixmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
		 pixmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
  glBindTexture(GL_TEXTURE_2D, boundTexture);

  if (inState->glObjects) {
    dlKey = (__glcDisplayListKey *)__glcMalloc(sizeof(__glcDisplayListKey));
    if (!dlKey) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(pixmap.buffer);
      __glcFaceDescClose(inFont->faceDesc);
      return;
    }

    /* Initialize the key */
    dlKey->list = glGenLists(1);
    dlKey->faceDesc = inFont->faceDesc;
    dlKey->code = inCode;
    dlKey->renderMode = GLC_TEXTURE;

    /* Get (or create) a new entry that contains the display list and store
     * the key in it
     */
    node = (FT_ListNode)dlKey;
    node->data = dlKey;
    FT_List_Add(&inFont->parent->displayList, node);

    /* Create the display list */
    glNewList(dlKey->list, GL_COMPILE_AND_EXECUTE);
  }

  glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  /* Repeat glBindTexture() so that the display list includes it */
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  /* Compute the size of the glyph */
  width = (GLfloat)((boundBox.xMax - boundBox.xMin) / 64.);
  height = (GLfloat)((boundBox.yMax - boundBox.yMin) / 64.);

  /* Do the actual GL rendering */
  glBegin(GL_QUADS);
  glTexCoord2f(0., 0.);
  glVertex2f(boundBox.xMin / 64. / GLC_POINT_SIZE, 
	     boundBox.yMin / 64. / GLC_POINT_SIZE);
  glTexCoord2f(width / pixmap.width, 0.);
  glVertex2f(boundBox.xMax / 64. / GLC_POINT_SIZE,
	     boundBox.yMin / 64. / GLC_POINT_SIZE);
  glTexCoord2f(width / pixmap.width, height / pixmap.rows);
  glVertex2f(boundBox.xMax / 64. / GLC_POINT_SIZE,
	     boundBox.yMax / 64. / GLC_POINT_SIZE);
  glTexCoord2f(0., height / pixmap.rows);
  glVertex2f(boundBox.xMin / 64. / GLC_POINT_SIZE,
	     boundBox.yMax / 64. / GLC_POINT_SIZE);
  glEnd();

  /* Store the glyph advance in the display list */
  glTranslatef(face->glyph->advance.x / 64. / GLC_POINT_SIZE, 
	       face->glyph->advance.y / 64. / GLC_POINT_SIZE, 0.);

  glPopAttrib();

  if (inState->glObjects) {
    /* Finish display list creation */
    glEndList();
  }
  else {
    if (!inDisplayListIsBuilding)
      glDeleteTextures(1, &texture);
  }

  __glcFree(pixmap.buffer);
  __glcFaceDescClose(inFont->faceDesc);
}

static FT_Error __glcDisplayListIterator(FT_ListNode node, void *user)
{
  __glcDisplayListKey *dlKey = (__glcDisplayListKey*)user;
  __glcDisplayListKey *nodeKey = (__glcDisplayListKey*)node;

  if ((dlKey->faceDesc == nodeKey->faceDesc) && (dlKey->code == nodeKey->code)
      && (dlKey->renderMode == nodeKey->renderMode))
    dlKey->list = nodeKey->list;

  return 0;
}

/* This internal function look in the linked list of display lists for a
 * display list that draws the desired glyph in the desired rendering mode
 * using the desired font. Returns GL_TRUE if a display list exists, returns
 * GL_FALSE otherwise.
 */
static GLboolean __glcFindDisplayList(__glcFont *inFont, GLint inCode,
				      GLCenum renderMode)
{
  __glcDisplayListKey dlKey;

  /* Initialize the key */
  dlKey.faceDesc = inFont->faceDesc;
  dlKey.code = inCode;
  dlKey.renderMode = renderMode;
  dlKey.list = 0;
  /* Look for the key in the display list linked list */
  FT_List_Iterate(&inFont->parent->displayList,
		  __glcDisplayListIterator, &dlKey);
  /* If a display list has been found then display it */
  if (dlKey.list) {
    glCallList(dlKey.list);
    return GL_TRUE;
  }

  return GL_FALSE;
}

/* Internal function that is called to do the actual rendering :
 * 'inCode' must be given in UCS-4 format
 */
static void __glcRenderChar(GLint inCode, GLint inFont,
			    __glcContextState* inState)
{
  __glcFont* font = NULL;
  FT_UInt glyphIndex = 0;
  FT_ListNode node = NULL;
  FT_Face face = NULL;
  GLdouble transformMatrix[16];
  GLint listIndex = 0;
  GLboolean displayListIsBuilding = GL_FALSE;

  for (node = inState->fontList.head; node; node = node->next) {
    font = (__glcFont*)node->data;
    assert(font);
    if (font->id == inFont) break;
  }

  if (!node)
    return;

  face = __glcFaceDescOpen(font->faceDesc, inState);
  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Get and load the glyph which unicode code is identified by inCode */
  glyphIndex = __glcCharMapGlyphIndex(font->charMap, face, inCode);
  if (!glyphIndex) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcFaceDescClose(font->faceDesc);
    return;
  }

  if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP |
		    FT_LOAD_IGNORE_TRANSFORM)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFaceDescClose(font->faceDesc);
    return;
  }

  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  displayListIsBuilding = listIndex || inState->glObjects;

  if (inState->renderStyle != GLC_BITMAP) {
    GLdouble projectionMatrix[16];
    GLdouble modelviewMatrix[16];
    int i = 0, j = 0;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

    /* Compute the matrix that transforms object space coordinates to viewport
     * coordinates. If we plan to use object space coordinates, this matrix is
     * set to identity.
     */
    for (i = 0; i < 4 ; i++) {
      for (j = 0; j < 4; j++) {
	transformMatrix[i*4+j] =
		modelviewMatrix[i*4+0]*projectionMatrix[0*4+j]+
		modelviewMatrix[i*4+1]*projectionMatrix[1*4+j]+
		modelviewMatrix[i*4+2]*projectionMatrix[2*4+j]+
		modelviewMatrix[i*4+3]*projectionMatrix[3*4+j];
      }
    }

    if (!displayListIsBuilding) {
      FT_Outline outline;
      FT_Vector* vector = NULL;
      GLdouble xMin = 1E20, yMin = 1E20, zMin = 1E20;
      GLdouble xMax = -1E20, yMax = -1E20, zMax = -1E20;

      /* Compute the bounding box of the control points of the glyph in the observer coordinates */
      outline = face->glyph->outline;
      if (!outline.n_points)
	return;

      for (vector = outline.points; vector < outline.points + outline.n_points; vector++) {
	GLdouble vx = vector->x / 64. / GLC_POINT_SIZE;
	GLdouble vy = vector->y / 64. / GLC_POINT_SIZE;
	GLdouble w = vx * transformMatrix[3] + vy * transformMatrix[7] + transformMatrix[15];
	GLdouble x = (vx * transformMatrix[0] + vy * transformMatrix[4] + transformMatrix[12]) / w;
	GLdouble y = (vx * transformMatrix[1] + vy * transformMatrix[5] + transformMatrix[13]) / w;
	GLdouble z = (vx * transformMatrix[2] + vy * transformMatrix[6] + transformMatrix[14]) / w;

	xMin = (x < xMin ? x : xMin);
	xMax = (x > xMax ? x : xMax);
	yMin = (y < yMin ? y : yMin);
	yMax = (y > yMax ? y : yMax);
	zMin = (z < zMin ? z : zMin);
	zMax = (z > zMax ? z : zMax);
      }

      /* If the bounding box of the glyph lies out of viewport then skip the glyph */
      if ((xMin > 1.) || (xMax < -1.) || (yMin > 1.) || (yMax < -1.) || (zMin > 1.) || (zMax < -1.))
	return;
    }
  }

  /* Call the appropriate function depending on the rendering mode. It first
   * checks if a display list that draws the desired glyph has already been
   * defined
   */
  switch(inState->renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(font, inState, glyphIndex);
    break;
  case GLC_TEXTURE:
    if (!__glcFindDisplayList(font, inCode, GLC_TEXTURE))
      __glcRenderCharTexture(font, inState, inCode, displayListIsBuilding);
    break;
  case GLC_LINE:
  case GLC_TRIANGLE:
    if (!__glcFindDisplayList(font, inCode, inState->renderStyle))
      __glcRenderCharScalable(font, inState, inCode, inState->renderStyle, face,
			      displayListIsBuilding, transformMatrix);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
  }

  __glcFaceDescClose(font->faceDesc);
}

/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code and '<hexcode>' format
 * are issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
static void __glcProcessChar(__glcContextState *inState, GLint inCode)
{  
  GLint repCode = 0;
  GLint font = 0;
  
  /* Get a font that maps inCode */
  font = __glcCtxGetFont(inState, inCode);
  if (font >= 0) {
    /* A font has been found */
    __glcRenderChar(inCode, font, inState);
    return;
  }

  /* __glcCtxGetFont() can not find a font that maps inCode, we then attempt to
   * produce an alternate rendering.
   */

  /* If the variable GLC_REPLACEMENT_CODE is nonzero, and __glcCtxGetFont()
   * finds a font that maps the replacement code, we now render the character
   * that the replacement code is mapped to
   */
  repCode = inState->replacementCode;
  font = __glcCtxGetFont(inState, repCode);
  if (repCode && font) {
    __glcRenderChar(repCode, font, inState);
    return;
  }
  else {
    /* If we get there, we failed to render both the character that inCode maps
     * to and the replacement code. Now, we will try to render the character
     * sequence "\<hexcode>", where '\' is the character REVERSE SOLIDUS 
     * (U+5C), '<' is the character LESS-THAN SIGN (U+3C), '>' is the character
     * GREATER-THAN SIGN (U+3E), and 'hexcode' is inCode represented as a
     * sequence of hexadecimal digits. The sequence has no leading zeros, and
     * alphabetic digits are in upper case. The GLC measurement commands treat
     * the sequence as a single character.
     */
    char buf[10];
    GLint i = 0;
    GLint n = 0;

    /* Check if a font maps to '\', '<' and '>'. */
    if (!__glcCtxGetFont(inState, '\\') || !__glcCtxGetFont(inState, '<')
	|| !__glcCtxGetFont(inState, '>'))
      return;

    /* Check if a font maps hexadecimal digits */
    sprintf(buf,"%X", (int)inCode);
    n = strlen(buf);
    for (i = 0; i < n; i++) {
      if (!__glcCtxGetFont(inState, buf[i]))
	return;
    }

    /* Render the '\<hexcode>' sequence */
    __glcRenderChar('\\', __glcCtxGetFont(inState, '\\'), inState);
    __glcRenderChar('<', __glcCtxGetFont(inState, '<'), inState);
    for (i = 0; i < n; i++)
      __glcRenderChar(buf[i], __glcCtxGetFont(inState, buf[i]), inState);
    __glcRenderChar('>', __glcCtxGetFont(inState, '>'), inState);
  }
}

/** \ingroup render
 *  This command renders the character that \e inCode is mapped to.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if the current string stype is
 *  \b GLC_UTF8_QSO.
 *  \param inCode The character to render
 *  \sa glcRenderString()
 *  \sa glcRenderCountedString()
 *  \sa glcReplacementCode()
 *  \sa glcRenderStyle()
 *  \sa glcCallbackFunc()
 */
void glcRenderChar(GLint inCode)
{
  __glcContextState *state = NULL;
  GLint code = 0;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the character code converted to the UCS-4 format.
   * See comments at the definition of __glcConvertGLintToUcs4() for known
   * limitations
   */
  code = __glcConvertGLintToUcs4(state, inCode);
  if (code < 0)
    return;

  __glcProcessChar(state, code);
}
  
/** \ingroup render
 *  This command is identical to the command glcRenderChar(), except that it
 *  renders a string of characters. The string comprises the first \e inCount
 *  elements of the array \e inString, which need not be followed by a zero
 *  element.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inCount is less than zero.
 *  \param inCount The number of elements in the string to be rendered
 *  \param inString The array of characters from which to render \e inCount
 *                  elements.
 *  \sa glcRenderChar()
 *  \sa glcRenderString()
 */
void glcRenderCountedString(GLint inCount, const GLCchar *inString)
{
  GLint i = 0;
  __glcContextState *state = NULL;
  FcChar8* UinString = NULL;
  FcChar8* ptr = NULL;

  /* Check if inCount is positive */
  if (inCount < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertCountedStringToUtf8(inCount, inString,
					      state->stringType);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Render the string */
  ptr = UinString;
  for (i = 0; i < inCount; i++) {
    FcChar32 code = 0;
    int len = 0;
    len = FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    if (len < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      break;
    }
    ptr += len;
    __glcProcessChar(state, code);
  }

  __glcFree(UinString);
}

/** \ingroup render
 *  This command is identical to the command glcRenderCountedString(), except
 *  that \e inString is zero terminated, not counted.
 *  \param inString A zero-terminated string of characters.
 *  \sa glcRenderChar()
 *  \sa glcRenderCountedString()
 */
void glcRenderString(const GLCchar *inString)
{
  __glcContextState *state = NULL;
  FcChar8* UinString = NULL;
  FcChar8* ptr = NULL;
  FcChar32 code = 0;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertToUtf8(inString, state->stringType);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Render the string */
  ptr = UinString;
  do {
    int len = 0;
    len = FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    if (len < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      break;
    }
    ptr += len;
    __glcProcessChar(state, code);
  } while (*ptr);

  __glcFree(UinString);
}

/** \ingroup render
 *  This command assigns the value \e inStyle to the variable
 *  \b GLC_RENDER_STYLE. Legal values for \e inStyle are defined in the table
 *  below :
 *  <center>
 *  <table>
 *  <caption>Rendering styles</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_BITMAP</b></td> <td>0x0100</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_LINE</b></td> <td>0x0101</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_TEXTURE</b></td> <td>0x0102</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_TRIANGLE</b></td> <td>0x0103</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inStyle The value to assign to the variable \b GLC_RENDER_STYLE.
 *  \sa glcGeti() with argument \b GLC_RENDER_STYLE
 */
void glcRenderStyle(GLCenum inStyle)
{
  __glcContextState *state = NULL;

  /* Check if inStyle has a legal value */
  switch(inStyle) {
  case GLC_BITMAP:
  case GLC_LINE:
  case GLC_TEXTURE:
  case GLC_TRIANGLE:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a current state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the rendering style */
  state->renderStyle = inStyle;
  return;
}

/** \ingroup render
 *  This command assigns the value \e inCode to the variable
 *  \b GLC_REPLACEMENT_CODE. The replacement code is the code which is used
 *  whenever glcRenderChar() can not find a font that owns a character which
 *  the parameter \e inCode of glcRenderChar() maps to.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if the current string stype is
 *  \b GLC_UTF8_QSO.
 *  \param inCode An integer to assign to \b GLC_REPLACEMENT_CODE.
 *  \sa glcGeti() with argument \b GLC_REPLACEMENT_CODE
 *  \sa glcRenderChar()
 */
void glcReplacementCode(GLint inCode)
{
  __glcContextState *state = __glcGetCurrent();
  GLint code = 0;

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the replacement character converted to the UCS-4 format.
   * See comments at the definition of __glcConvertGLintToUcs4() for known
   * limitations
   */
  code = __glcConvertGLintToUcs4(state, inCode);
  if (code < 0)
    return;
  
  /* Stores the replacement code */
  state->replacementCode = code;
  return;
}

/** \ingroup render
 *  This command assigns the value \e inVal to the variable \b GLC_RESOLUTION.
 *  It is used to compute the size of characters in pixels from the size in
 *  points.
 *
 *  The resolution is given in \e dpi (dots per inch). If \e inVal is zero, the
 *  resolution defaults to 72 dpi.
 *  \param inVal A floating point number to be used as resolution.
 *  \sa glcGeti() with argument GLC_RESOLUTION
 */
void glcResolution(GLfloat inVal)
{
  __glcContextState *state = __glcGetCurrent();
  FT_ListNode node = NULL;

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  state->resolution = inVal;

  /* Modify the resolution of every font of the GLC_CURRENT_FONT_LIST */
  for (node = state->currentFontList.head; node; node = node->next) {
    assert(node->data);
    __glcFaceDescSetCharSize(((__glcFont*)node->data)->faceDesc,
			     (FT_UInt)inVal);
  }

  return;
}
