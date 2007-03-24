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

#include "internal.h"

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "texture.h"
#include FT_OUTLINE_H
#include FT_LIST_H



/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(FT_GlyphSlot inGlyph,
				  __GLCcontext* inContext, GLfloat scale_x,
				  GLfloat scale_y, GLboolean inIsRTL)
{
  FT_Matrix matrix;
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_Bitmap pixmap;
  GLfloat *transform = inContext->bitmapMatrix;

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

  /* Calculate pitch to upper 8 byte boundary for 1 bit / pixel, i.e. ceil() */
  pixmap.pitch = (boundingBox.xMax - boundingBox.xMin + 511) >> 9;
  pixmap.width = pixmap.pitch << 3;
  pixmap.rows = (boundingBox.yMax - boundingBox.yMin) >> 6;

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
  FT_Outline_Translate(&outline, -boundingBox.xMin, -boundingBox.yMin);

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inContext->library, &outline, &pixmap)) {
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

  if (inIsRTL) {
    glBitmap(0, 0, 0, 0,
	     inGlyph->advance.y / 64. * matrix.xy / 65536.
	     - inGlyph->advance.x / 64. * matrix.xx / 65536.,
	     inGlyph->advance.y / 64. * matrix.yy / 65536.
	     - inGlyph->advance.x / 64. * matrix.yx / 65536.,
	     NULL);
    glBitmap(pixmap.width, pixmap.rows, -boundingBox.xMin >> 6,
	     -boundingBox.yMin >> 6, 0., 0., pixmap.buffer);
  }
  else
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



/* Internal function that is called to do the actual rendering :
 * 'inCode' must be given in UCS-4 format
 */
static void* __glcRenderChar(GLint inCode, GLint inPrevCode, GLboolean inIsRTL,
			    __GLCfont* inFont, __GLCcontext* inContext,
                            void* GLC_UNUSED_ARG(inData),
			    GLboolean GLC_UNUSED_ARG(inMultipleChars))
{
  GLfloat transformMatrix[16];
  GLfloat scale_x = GLC_POINT_SIZE;
  GLfloat scale_y = GLC_POINT_SIZE;
  __GLCglyph* glyph = NULL;
  GLboolean displayListIsBuilding = GL_FALSE;
  FT_Face face = NULL;
  GLfloat sx64 = 0., sy64 = 0.;

  assert(inFont);

  displayListIsBuilding = __glcGetScale(inContext, transformMatrix, &scale_x,
                                       &scale_y);

  if ((scale_x == 0.f) || (scale_y == 0.f))
    return NULL;

  if (inPrevCode && inContext->enableState.kerning) {
    GLfloat kerning[2];
    GLint leftCode = inIsRTL ? inCode : inPrevCode;
    GLint rightCode = inIsRTL ? inPrevCode : inCode;

    if (__glcFontGetKerning(inFont, leftCode, rightCode, kerning, inContext,
			    scale_x, scale_y)) {
      if (inIsRTL)
	kerning[0] = -kerning[0];

      if (inContext->renderState.renderStyle == GLC_BITMAP)
	glBitmap(0, 0, 0, 0,
		 kerning[0] * inContext->bitmapMatrix[0] 
		 + kerning[1] * inContext->bitmapMatrix[2],
		 kerning[0] * inContext->bitmapMatrix[1]
		 + kerning[1] * inContext->bitmapMatrix[3],
		 NULL);
      else
	glTranslatef(kerning[0], kerning[1], 0.);
    }
  }

  /* Get and load the glyph which unicode code is identified by inCode */
  glyph = __glcFontGetGlyph(inFont, inCode, inContext);

  /* Renders the display lists if they exist and if GLC_GL_OBJECTS is enabled
   * then return.
   */
  if (inContext->enableState.glObjects) {
    if (inIsRTL)
      glTranslatef(-glyph->advance[0], glyph->advance[1], 0.);

    switch(inContext->renderState.renderStyle) {
    case GLC_TEXTURE:
      if (glyph->displayList[0]) {
	glCallList(glyph->displayList[0]);
	/* The glyph is put at the head of the atlas list, so that we keep track
	 * of glyphes that are used often in order not to remove those ones from
	 * the atlas when it is full.
	 */
	FT_List_Up(&inContext->atlasList, (FT_ListNode)glyph->textureObject);
	if (!inIsRTL)
	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	return NULL;
      }
      break;
    case GLC_LINE:
      if (glyph->displayList[1]) {
	glCallList(glyph->displayList[1]);
	if (!inIsRTL)
	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	return NULL;
      }
      break;
    case GLC_TRIANGLE:
      if ((!inContext->enableState.extrude) && glyph->displayList[2]) {
	glCallList(glyph->displayList[2]);
	if (!inIsRTL)
	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	return NULL;
      }
      if (inContext->enableState.extrude && glyph->displayList[3]) {
	glCallList(glyph->displayList[3]);
	if (!inIsRTL)
	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	return NULL;
      }
      break;
    }
  }

#ifndef FT_CACHE_H
  face = inFont->faceDesc->face;
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)inFont->faceDesc,
			     &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
#endif

  __glcFaceDescLoadFreeTypeGlyph(inFont->faceDesc, inContext, scale_x, scale_y,
				 glyph->index);

  sx64 = 64. * scale_x;
  sy64 = 64. * scale_y;

  if (inContext->renderState.renderStyle != GLC_BITMAP) {
    FT_Outline outline= face->glyph->outline;

    /* If the outline contains no point then the glyph represents a space
     * character and there is no need to continue the process of rendering.
     */
    if (!outline.n_points) {
      /* Update the advance and return */
      if (inIsRTL)
        glTranslatef(-face->glyph->advance.x / sx64,
		     face->glyph->advance.y / sy64, 0.);
      else
        glTranslatef(face->glyph->advance.x / sx64,
		     face->glyph->advance.y / sy64, 0.);
      return NULL;
    }

    /* coordinates are given in 26.6 fixed point integer hence we
     * divide the scale by 2^6
     */
    if (!displayListIsBuilding) 
      glScalef(1. / sx64, 1. / sy64, 1.);

    if (inContext->enableState.glObjects) {
      glyph->advance[0] = (GLfloat) face->glyph->advance.x / sx64;
      glyph->advance[1] = (GLfloat) face->glyph->advance.y / sy64;
    }

    if (inIsRTL) {
      if (inContext->enableState.glObjects)
	glTranslatef(-glyph->advance[0], glyph->advance[1], 0.);
      else
	glTranslatef(-face->glyph->advance.x, face->glyph->advance.y, 0.);
    }
  }

  /* Call the appropriate function depending on the rendering mode. It first
   * checks if a display list that draws the desired glyph has already been
   * defined
   */
  switch(inContext->renderState.renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(face->glyph, inContext, scale_x, scale_y, inIsRTL);
    break;
  case GLC_TEXTURE:
      __glcRenderCharTexture(inFont, inContext, displayListIsBuilding,
			     scale_x, scale_y, glyph);
    break;
  case GLC_LINE:
      __glcRenderCharScalable(inFont, inContext, GLC_LINE,
			      displayListIsBuilding, transformMatrix,
			      scale_x, scale_y, glyph);
    break;
  case GLC_TRIANGLE:
      __glcRenderCharScalable(inFont, inContext, GLC_TRIANGLE,
			      displayListIsBuilding, transformMatrix,
			      scale_x, scale_y, glyph);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
  }

  if (inContext->renderState.renderStyle != GLC_BITMAP) {
    if (!inIsRTL) {
      if (inContext->enableState.glObjects)
	glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
      else
	glTranslatef(face->glyph->advance.x, face->glyph->advance.y, 0.);
    }
    if (!displayListIsBuilding)
      glScalef(sx64, sy64, 1.);
  }
#ifndef FT_CACHE_H
  __glcFaceDescClose(font->faceDesc);
#endif
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
void APIENTRY glcRenderChar(GLint inCode)
{
  __GLCcontext *ctx = NULL;
  GLint code = 0;
  __GLCglState GLState;
  __GLCcharacter prevCode = { 0, NULL };

  /* Check if the current thread owns a context state */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 0)
    return;

  /* Save the value of the GL parameters */
  __glcSaveGLState(&GLState, ctx, GL_FALSE);

  if (ctx->renderState.renderStyle == GLC_LINE ||
      ctx->renderState.renderStyle == GLC_TRIANGLE) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }

  /* Set the texture environment if the render style is GLC_TEXTURE */
  if (ctx->renderState.renderStyle == GLC_TEXTURE) {
    /* Set the new values of the parameters */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (ctx->enableState.glObjects && ctx->atlas.id)
      glBindTexture(GL_TEXTURE_2D, ctx->atlas.id);
    else if (!ctx->enableState.glObjects && ctx->texture.id) {
      glBindTexture(GL_TEXTURE_2D, ctx->texture.id);
      if (GLEW_ARB_pixel_buffer_object && ctx->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			ctx->texture.bufferObjectID);
    }
  }

  __glcProcessChar(ctx, code, &prevCode, GL_FALSE, __glcRenderChar, NULL);

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, ctx, GL_FALSE);
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
void APIENTRY glcRenderCountedString(GLint inCount, const GLCchar *inString)
{
  GLint i = 0;
  __GLCcontext *ctx = NULL;
  FcChar32* UinString = NULL;
  FcChar32* ptr = NULL;
  __GLCglState GLState;
  __GLCcharacter prevCode = { 0, NULL };
  GLboolean isRightToLeft = GL_FALSE;

  /* Check if inCount is positive */
  if (inCount < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a context state */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertCountedStringToVisualUcs4(ctx, &isRightToLeft,
						    inString, inCount);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Save the value of the GL parameters */
  __glcSaveGLState(&GLState, ctx, GL_FALSE);

  if (ctx->renderState.renderStyle == GLC_LINE ||
      ctx->renderState.renderStyle == GLC_TRIANGLE) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }

  /* Set the texture environment if the render style is GLC_TEXTURE */
  if (ctx->renderState.renderStyle == GLC_TEXTURE) {
    /* Set the new values of the parameters */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (ctx->enableState.glObjects && ctx->atlas.id)
      glBindTexture(GL_TEXTURE_2D, ctx->atlas.id);
    else if (!ctx->enableState.glObjects && ctx->texture.id) {
      glBindTexture(GL_TEXTURE_2D, ctx->texture.id);
      if (GLEW_ARB_pixel_buffer_object && ctx->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			ctx->texture.bufferObjectID);
    }
  }

  /* Render the string */
  ptr = UinString;
  for (i = 0; i < inCount; i++)
    __glcProcessChar(ctx, *(ptr++), &prevCode, isRightToLeft, __glcRenderChar,
		     NULL);

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, ctx, GL_FALSE);
}



/** \ingroup render
 *  This command is identical to the command glcRenderCountedString(), except
 *  that \e inString is zero terminated, not counted.
 *  \param inString A zero-terminated string of characters.
 *  \sa glcRenderChar()
 *  \sa glcRenderCountedString()
 */
void APIENTRY glcRenderString(const GLCchar *inString)
{
  __GLCcontext *ctx = NULL;
  FcChar32* UinString = NULL;
  FcChar32* ptr = NULL;
  __GLCglState GLState;
  __GLCcharacter prevCode = { 0, NULL };
  GLboolean isRightToLeft = GL_FALSE;

  /* Check if the current thread owns a context state */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertToVisualUcs4(ctx, &isRightToLeft, inString);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Save the value of the GL parameters */
  __glcSaveGLState(&GLState, ctx, GL_FALSE);

  if (ctx->renderState.renderStyle == GLC_LINE ||
      ctx->renderState.renderStyle == GLC_TRIANGLE) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }

  /* Set the texture environment if the render style is GLC_TEXTURE */
  if (ctx->renderState.renderStyle == GLC_TEXTURE) {
    /* Set the new values of the parameters */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (ctx->enableState.glObjects && ctx->atlas.id)
      glBindTexture(GL_TEXTURE_2D, ctx->atlas.id);
    else if (!ctx->enableState.glObjects && ctx->texture.id) {
      glBindTexture(GL_TEXTURE_2D, ctx->texture.id);
      if (GLEW_ARB_pixel_buffer_object && ctx->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			ctx->texture.bufferObjectID);
    }
  }

  /* Render the string */
  ptr = UinString;
  while (*ptr)
    __glcProcessChar(ctx, *(ptr++), &prevCode, isRightToLeft, __glcRenderChar,
		     NULL);

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, ctx, GL_FALSE);
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
void APIENTRY glcRenderStyle(GLCenum inStyle)
{
  __GLCcontext *ctx = NULL;

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
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the rendering style */
  ctx->renderState.renderStyle = inStyle;
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
void APIENTRY glcReplacementCode(GLint inCode)
{
  __GLCcontext *ctx = __glcGetCurrent();
  GLint code = 0;

  /* Check if the current thread owns a current state */
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the replacement character converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 0)
    return;

  /* Stores the replacement code */
  ctx->stringState.replacementCode = code;
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
void APIENTRY glcResolution(GLfloat inVal)
{
  __GLCcontext *ctx = __glcGetCurrent();

  /* Check if the current thread owns a current state */
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  ctx->renderState.resolution = inVal;

  return;
}
