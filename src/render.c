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
 * defined. If a callback function is defined for \b GLC_OP_glcUnmappedCode,
 * GLC calls the function. The callback function provides the character code to
 * the user and allows loading of the appropriate font. After the callback
 * returns, GLC tries to render the character code again.
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
#include <GL/gl.h>
#ifdef __WIN32__
/* GL_TEXTURE_MAX_LEVEL is defined in OpenGL 1.2 which is not supposed to be
 * available under Windows.
 */
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
#endif
#include <GL/glu.h>
#endif

#include "internal.h"
#include FT_OUTLINE_H
#include FT_LIST_H



/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(FT_GlyphSlot inGlyph,
				  __glcContextState* inState, GLfloat scale_x,
				  GLfloat scale_y)
{
  FT_Matrix matrix;
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_Bitmap pixmap;
  GLfloat *transform = inState->bitmapMatrix;

  /* compute glyph dimensions */
  matrix.xx = (FT_Fixed)(transform[0] * 65536. / scale_x);
  matrix.xy = (FT_Fixed)(transform[2] * 65536. / scale_y);
  matrix.yx = (FT_Fixed)(transform[1] * 65536. / scale_x);
  matrix.yy = (FT_Fixed)(transform[3] * 65536. / scale_y);

  /* Get the bounding box of the glyph */
  outline = inGlyph->outline;
  FT_Outline_Transform(&outline, &matrix);
  FT_Outline_Get_CBox(&outline, &boundingBox);

  /* Compute the sizes of the pixmap where the glyph will be drawn */
  boundingBox.xMin = boundingBox.xMin & -64;	/* floor(xMin) */
  boundingBox.yMin = boundingBox.yMin & -64;	/* floor(yMin) */
  boundingBox.xMax = (boundingBox.xMax + 63) & -64;	/* ceiling(xMax) */
  boundingBox.yMax = (boundingBox.yMax + 63) & -64;	/* ceiling(yMax) */

  pixmap.width = (boundingBox.xMax - boundingBox.xMin) >> 6;
  pixmap.rows = (boundingBox.yMax - boundingBox.yMin) >> 6;
  pixmap.pitch = ((pixmap.width + 4) >> 3) + 1;	/* 1 bit / pixel */
  pixmap.width = pixmap.pitch << 3;

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
  pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
  if (!pixmap.buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* fill the pixmap buffer with the background color */
  memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);

  /* Flip the picture */
  pixmap.pitch = - pixmap.pitch;

  /* translate the outline to match (0,0) with the glyph's lower left corner */
  FT_Outline_Translate(&outline, -(boundingBox.xMin & -64),
		       -(boundingBox.yMin & -64));

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inState->library, &outline, &pixmap)) {
    __glcFree(pixmap.buffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Do the actual GL rendering */
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glBitmap(pixmap.width, pixmap.rows, -boundingBox.xMin >> 6,
	   -boundingBox.yMin >> 6,
	   inGlyph->advance.x / 64. * matrix.xx / 65536.
	   + inGlyph->advance.y / 64. * matrix.xy / 65536.,
	   inGlyph->advance.x / 64. * matrix.yx / 65536.
	   + inGlyph->advance.y / 64. * matrix.yy / 65536.,
	   pixmap.buffer);

  glPopClientAttrib();

  __glcFree(pixmap.buffer);
}



/* Compute the lower power of 2 that is greater than value. It is used to
 * determine the smaller texture than can contain a glyph.
 */
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
				   __glcContextState* inState,
				   GLboolean inDisplayListIsBuilding,
				   GLfloat scale_x, GLfloat scale_y,
				   __glcGlyph* inGlyph)
{
  FT_Face face = inFont->faceDesc->face;
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_Bitmap pixmap;
  GLuint texture = 0;
  GLfloat width = 0, height = 0;
  GLint format = 0;
  GLint boundTexture = 0;
  GLint level = 0;

  assert(face);

  outline = face->glyph->outline;

  /* Get the bounding box of the glyph */
  FT_Outline_Get_CBox(&outline, &boundingBox);

  /* Compute the size of the pixmap where the glyph will be rendered */
  boundingBox.xMin = boundingBox.xMin & -64;	/* floor(xMin) */
  boundingBox.yMin = boundingBox.yMin & -64;	/* floor(yMin) */
  boundingBox.xMax = (boundingBox.xMax + 63) & -64;	/* ceiling(xMax) */
  boundingBox.yMax = (boundingBox.yMax + 63) & -64;	/* ceiling(yMax) */

  pixmap.width = __glcNextPowerOf2(((boundingBox.xMax - boundingBox.xMin) >> 6)
				   + 1);
  pixmap.rows = __glcNextPowerOf2(((boundingBox.yMax - boundingBox.yMin) >> 6)
				  + 1);
  pixmap.pitch = pixmap.width;		/* 8 bits / pixel */

  /* Check if a new texture can be created */
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_ALPHA8, pixmap.width,
	       pixmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS,
			   &format);
  if (!format) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Fill the pixmap descriptor and the pixmap buffer */
  pixmap.pixel_mode = ft_pixel_mode_grays;	/* Anti-aliased rendering */
  pixmap.num_grays = 256;
  pixmap.buffer = (GLubyte *)__glcMalloc(pixmap.rows * pixmap.pitch);
  if (!pixmap.buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Flip the picture */
  pixmap.pitch = - pixmap.pitch;

  /* Create a texture object and make it current */
  glGenTextures(1, &texture);

  /* Create the texture */
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
  glBindTexture(GL_TEXTURE_2D, texture);

  /* Iterate on the powers of 2 in order to build the mipmap */
  while ((pixmap.width > 2) && (pixmap.rows > 2)) {
    FT_Vector* vector = NULL;

    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, - pixmap.rows * pixmap.pitch);

    /* translate the outline to match (0,0) with the glyph's lower left
     * corner
     */
    FT_Outline_Translate(&outline, -((boundingBox.xMin >> level) & -64),
			 -((boundingBox.yMin >> level) & -64));

    /* render the glyph */
    if (FT_Outline_Get_Bitmap(inState->library, &outline, &pixmap)) {
      glBindTexture(GL_TEXTURE_2D, boundTexture);
      glPopClientAttrib();
      glDeleteTextures(1, &texture);
      __glcFree(pixmap.buffer);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA8, pixmap.width,
		 pixmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixmap.buffer);

    /* A mipmap is built only if a display list is currently building
     * otherwise it adds useless computations
     */
    if ((!inState->mipmap) || (!inDisplayListIsBuilding))
      break;

    /* translate the outline to match (0,0) with the glyph's lower left
     * corner
     */
    FT_Outline_Translate(&outline, ((boundingBox.xMin >> level) & -64),
			 ((boundingBox.yMin >> level) & -64));

    level++; /* Next level of mipmap */
    pixmap.width >>= 1;
    pixmap.rows >>= 1;
    pixmap.pitch >>= 1;

    /* For the next level of mipmap, the character size must divided by 2. */
    for (vector = outline.points; vector < outline.points + outline.n_points;
	 vector++) {
      vector->x >>= 1;
      vector->y >>= 1;
    }

  }

  /* Initialize the texture parameters */
  if (inState->mipmap && inDisplayListIsBuilding) {
    pixmap.width <<= level;
    pixmap.rows <<= level;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		    GL_LINEAR_MIPMAP_LINEAR);
  }
  else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glBindTexture(GL_TEXTURE_2D, boundTexture);
  glPopClientAttrib();

  /* Add the new texture to the texture list and the new display list
   * to the list of display lists
   */
  if (inState->glObjects) {
    inGlyph->textureObject = texture;
    inGlyph->displayList[0] = glGenLists(1);
    if (!inGlyph->displayList[0]) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      glDeleteTextures(1, &texture);
      __glcFree(pixmap.buffer);
      return;
    }

    /* Create the display list */
    glNewList(inGlyph->displayList[0], GL_COMPILE_AND_EXECUTE);
  }

  glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  /* Repeat glBindTexture() so that the display list includes it */
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  /* Compute the size of the glyph */
  width = (GLfloat)((boundingBox.xMax - boundingBox.xMin) / 64.);
  height = (GLfloat)((boundingBox.yMax - boundingBox.yMin) / 64.);

  /* Do the actual GL rendering */
  glBegin(GL_QUADS);
  glTexCoord2f(0., 0.);
  glVertex2f(boundingBox.xMin / 64. / scale_x, 
	     boundingBox.yMin / 64. / scale_y);
  glTexCoord2f(width / pixmap.width, 0.);
  glVertex2f(boundingBox.xMax / 64. / scale_x,
	     boundingBox.yMin / 64. / scale_y);
  glTexCoord2f(width / pixmap.width, height / pixmap.rows);
  glVertex2f(boundingBox.xMax / 64. / scale_x,
	     boundingBox.yMax / 64. / scale_y);
  glTexCoord2f(0., height / pixmap.rows);
  glVertex2f(boundingBox.xMin / 64. / scale_x,
	     boundingBox.yMax / 64. / scale_y);
  glEnd();

  /* Store the glyph advance in the display list */
  glTranslatef(face->glyph->advance.x / 64. / scale_x,
	       face->glyph->advance.y / 64. / scale_y, 0.);

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
}



/* Internal function that is called to do the actual rendering :
 * 'inCode' must be given in UCS-4 format
 */
static void* __glcRenderChar(GLint inCode, GLint inFont,
			    __glcContextState* inState, void* inData,
			    GLboolean inMultipleChars)
{
  GLfloat transformMatrix[16];
  GLfloat scale_x = GLC_POINT_SIZE;
  GLfloat scale_y = GLC_POINT_SIZE;
  __glcGlyph* glyph = NULL;
  GLboolean displayListIsBuilding = __glcGetScale(inState, transformMatrix,
						  &scale_x, &scale_y);
  __glcFont* font = __glcVerifyFontParameters(inFont);

  if ((!font) || (scale_x == 0.f) || (scale_y == 0.f))
    return NULL;

  /* Get and load the glyph which unicode code is identified by inCode */
  glyph = __glcFontGetGlyph(font, inCode, inState);
  __glcFaceDescLoadFreeTypeGlyph(font->faceDesc, inState, scale_x, scale_y,
                        glyph->index);

  if ((inState->renderStyle != GLC_BITMAP) && (!displayListIsBuilding)) {
    FT_Outline outline;
    FT_Vector* vector = NULL;
    GLfloat xMin = 1E20, yMin = 1E20, zMin = 1E20;
    GLfloat xMax = -1E20, yMax = -1E20, zMax = -1E20;
    FT_Face face = font->faceDesc->face;

    /* If the outline contains no point then the glyph represents a space
     * character and there is no need to continue the process of rendering.
     */
    outline = face->glyph->outline;
    if (!outline.n_points) {
      /* Update the advance and return */
      glTranslatef(face->glyph->advance.x / 64. / scale_x,
		  face->glyph->advance.y / 64. / scale_y, 0.);
      __glcFaceDescClose(font->faceDesc);
      return NULL;
    }

    /* Compute the bounding box of the control points of the glyph in the
     * observer coordinates.
     */
    for (vector = outline.points; vector < outline.points + outline.n_points;
	 vector++) {
      GLfloat vx = vector->x / 64. / scale_x;
      GLfloat vy = vector->y / 64. / scale_y;
      GLfloat w = vx * transformMatrix[3] + vy * transformMatrix[7]
		  + transformMatrix[15];
      GLfloat x = (vx * transformMatrix[0] + vy * transformMatrix[4]
		   + transformMatrix[12]) / w;
      GLfloat y = (vx * transformMatrix[1] + vy * transformMatrix[5]
		   + transformMatrix[13]) / w;
      GLfloat z = (vx * transformMatrix[2] + vy * transformMatrix[6]
		   + transformMatrix[14]) / w;

      xMin = (x < xMin ? x : xMin);
      xMax = (x > xMax ? x : xMax);
      yMin = (y < yMin ? y : yMin);
      yMax = (y > yMax ? y : yMax);
      zMin = (z < zMin ? z : zMin);
      zMax = (z > zMax ? z : zMax);
    }

    /* If the bounding box of the glyph lies out of viewport then skip the
     * glyph
     */
    if ((xMin > 1.) || (xMax < -1.) || (yMin > 1.) || (yMax < -1.)
	|| (zMin > 1.) || (zMax < -1.)) {
      glTranslatef(face->glyph->advance.x / 64. / scale_x,
		  face->glyph->advance.y / 64. / scale_y, 0.);
      __glcFaceDescClose(font->faceDesc);
      return NULL;
    }
  }

  /* Call the appropriate function depending on the rendering mode. It first
   * checks if a display list that draws the desired glyph has already been
   * defined
   */
  switch(inState->renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(font->faceDesc->face->glyph, inState, scale_x,
			  scale_y);
    break;
  case GLC_TEXTURE:
    if (glyph->displayList[0])
      glCallList(glyph->displayList[0]);
    else
      __glcRenderCharTexture(font, inState, displayListIsBuilding,
			     scale_x, scale_y, glyph);
    break;
  case GLC_LINE:
    if (glyph->displayList[1])
      glCallList(glyph->displayList[1]);
    else
      __glcRenderCharScalable(font, inState, GLC_LINE,
			      displayListIsBuilding, transformMatrix,
			      scale_x, scale_y, glyph);
    break;
  case GLC_TRIANGLE:
    if (glyph->displayList[2])
      glCallList(glyph->displayList[2]);
    else
      __glcRenderCharScalable(font, inState, GLC_TRIANGLE,
			      displayListIsBuilding, transformMatrix,
			      scale_x, scale_y, glyph);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
  }

  __glcFaceDescClose(font->faceDesc);
  return NULL;
}



/** \ingroup render
 *  This command renders the character that \e inCode is mapped to.
 *  \param inCode The character to render
 *  \sa glcRenderString()
 *  \sa glcRenderCountedString()
 *  \sa glcReplacementCode()
 *  \sa glcRenderStyle()
 *  \sa glcCallbackFunc()
 */
GLAPI void APIENTRY glcRenderChar(GLint inCode)
{
  __glcContextState *state = NULL;
  GLint code = 0;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(state, inCode);
  if (code < 0)
    return;

  __glcProcessChar(state, code, __glcRenderChar, NULL);
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
GLAPI void APIENTRY glcRenderCountedString(GLint inCount, const GLCchar *inString)
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

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

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
    __glcProcessChar(state, code, __glcRenderChar, NULL);
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
GLAPI void APIENTRY glcRenderString(const GLCchar *inString)
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

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

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
  while (*ptr) {
    int len = 0;
    len = FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    if (len < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      break;
    }
    ptr += len;
    __glcProcessChar(state, code, __glcRenderChar, NULL);
  }

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
GLAPI void APIENTRY glcRenderStyle(GLCenum inStyle)
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
 *  \param inCode An integer to assign to \b GLC_REPLACEMENT_CODE.
 *  \sa glcGeti() with argument \b GLC_REPLACEMENT_CODE
 *  \sa glcRenderChar()
 */
GLAPI void APIENTRY glcReplacementCode(GLint inCode)
{
  __glcContextState *state = __glcGetCurrent();
  GLint code = 0;

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the replacement character converted to the UCS-4 format */
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
GLAPI void APIENTRY glcResolution(GLfloat inVal)
{
  __glcContextState *state = __glcGetCurrent();

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  state->resolution = inVal;

  return;
}
