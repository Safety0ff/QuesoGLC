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



/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(__GLCfont* inFont, __GLCcontext* inContext,
                                  GLfloat scale_x, GLfloat scale_y,
                                  GLfloat* advance, GLboolean inIsRTL)
{
  GLfloat *transform = inContext->bitmapMatrix;
  GLint pixWidth = 0, pixHeight = 0;
  void* pixBuffer = NULL;
  GLint boundingBox[4] = {0, 0, 0, 0};

  __glcFaceDescGetBitmapSize(inFont->faceDesc, &pixWidth, &pixHeight,
                             boundingBox, scale_x, scale_y, 0, inContext);

  pixBuffer = (GLubyte *)__glcMalloc(pixWidth * pixHeight);
  if (!pixBuffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* render the glyph */
  if (!__glcFaceDescGetBitmap(inFont->faceDesc, pixWidth, pixHeight,
                              pixBuffer, inContext)) {
    __glcFree(pixBuffer);
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
	     advance[1] * transform[2] - advance[0] * transform[0],
	     advance[1] * transform[3] - advance[0] * transform[1],
	     NULL);
    glBitmap(pixWidth, pixHeight, -boundingBox[0] >> 6,
	     -boundingBox[1] >> 6, 0., 0., pixBuffer);
  }
  else
    glBitmap(pixWidth, pixHeight, -boundingBox[0] >> 6,
	     -boundingBox[1] >> 6,
	     advance[0] * transform[0] + advance[1] * transform[2],
	     advance[0] * transform[1] + advance[1] * transform[3],
	     pixBuffer);

  glPopClientAttrib();

  __glcFree(pixBuffer);
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
  GLfloat sx64 = 0., sy64 = 0.;
  GLfloat advance[2] = {0., 0.};

  assert(inFont);

  __glcGetScale(inContext, transformMatrix, &scale_x, &scale_y);

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

#ifndef GLC_FT_CACHE
  if (!__glcFaceDescGetAdvance(inFont->faceDesc, glyph->index, advance, scale_x,
                               scale_y, GL_FALSE, inContext))
    return NULL;
#else
  if (!__glcFaceDescGetAdvance(inFont->faceDesc, glyph->index, advance, scale_x,
                               scale_y, inContext))
    return NULL;
#endif

  sx64 = 64. * scale_x;
  sy64 = 64. * scale_y;

  if (inContext->renderState.renderStyle != GLC_BITMAP) {
    if (inIsRTL)
      glTranslatef(-advance[0], advance[1], 0.);

    /* If the outline contains no point then the glyph represents a space
     * character and there is no need to continue the process of rendering.
     */
    if (!__glcFaceDescOutlineEmpty(inFont->faceDesc, inContext)) {
      /* Update the advance and return */
      if (!inIsRTL)
        glTranslatef(advance[0], advance[1], 0.);
      if (inContext->enableState.glObjects) {
	glyph->advance[0] = advance[0];
	glyph->advance[1] = advance[1];
	glyph->isSpacingChar = GL_TRUE;
      }
      return NULL;
    }

    /* coordinates are given in 26.6 fixed point integer hence we
     * divide the scale by 2^6
     */
    if (!inContext->enableState.glObjects)
      glScalef(1. / sx64, 1. / sy64, 1.);
    else {
      glyph->advance[0] = advance[0];
      glyph->advance[1] = advance[1];
    }
  }

  /* Call the appropriate function depending on the rendering mode. It first
   * checks if a display list that draws the desired glyph has already been
   * defined
   */
  switch(inContext->renderState.renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(inFont, inContext, scale_x, scale_y, advance,
			  inIsRTL);
    break;
  case GLC_TEXTURE:
    __glcRenderCharTexture(inFont, inContext, scale_x, scale_y, glyph);
    break;
  case GLC_LINE:
    __glcRenderCharScalable(inFont, inContext, transformMatrix, scale_x,
			    scale_y, glyph);
    break;
  case GLC_TRIANGLE:
    __glcRenderCharScalable(inFont, inContext, transformMatrix, scale_x,
			    scale_y, glyph);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
  }

  if (inContext->renderState.renderStyle != GLC_BITMAP) {
    if (!inContext->enableState.glObjects)
      glScalef(sx64, sy64, 1.);
    if (!inIsRTL)
      glTranslatef(advance[0], advance[1], 0.);
  }
#ifndef GLC_FT_CACHE
  __glcFaceDescClose(inFont->faceDesc);
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
  GLint listIndex = 0;
  GLboolean saveGLObjects = GL_FALSE;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 32)
    return; /* Skip control characters and unknown characters */

  /* Disable the internal management of GL objects when the user is currently
   * building a display list.
   */
  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  if (listIndex) {
    saveGLObjects = ctx->enableState.glObjects;
    ctx->enableState.glObjects = GL_FALSE;
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
    if (ctx->enableState.glObjects && ctx->atlas.id) {
      glBindTexture(GL_TEXTURE_2D, ctx->atlas.id);
      if (GLEW_ARB_vertex_buffer_object && ctx->atlas.bufferObjectID) {
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, ctx->atlas.bufferObjectID);
	glInterleavedArrays(GL_T2F_V3F, 0, NULL);
      }
    }
    else if (!ctx->enableState.glObjects && ctx->texture.id) {
      glBindTexture(GL_TEXTURE_2D, ctx->texture.id);
      if (GLEW_ARB_pixel_buffer_object && ctx->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			ctx->texture.bufferObjectID);
    }
  }

  if (ctx->enableState.glObjects
      && ctx->renderState.renderStyle != GLC_BITMAP) {
    /* Renders the display lists if they exist */

    for (node = ctx->currentFontList.head; node; node = node->next) {
      __GLCfont* font = (__GLCfont*)node->data;
      __GLCglyph* glyph = __glcCharMapGetGlyph(font->charMap, code);

      if (glyph) {
	GLuint DLindex = ctx->renderState.renderStyle - 0x101;

	if (glyph->isSpacingChar) {
	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	  break;
	}

	if ((ctx->renderState.renderStyle == GLC_TRIANGLE)
	    && (ctx->enableState.extrude))
	  DLindex++;

	if (glyph->displayList[DLindex]) {

	  if (ctx->renderState.renderStyle == GLC_TEXTURE) {
	    if (GLEW_ARB_vertex_buffer_object)
	      glDrawArrays(GL_QUADS, 4 *
			   ((__GLCatlasElement*)glyph->textureObject)->position,
			   4);
	    else
	      glCallList(glyph->displayList[1]);
	    /* The glyph is put at the head of the atlas list, so that we keep
	     * track of glyphes that are used often in order not to remove those
	     * ones from the atlas when it is full.
	     */
	    FT_List_Up(&ctx->atlasList, (FT_ListNode)glyph->textureObject);
	  }
	  else
	    glCallList(glyph->displayList[DLindex]);

	  glTranslatef(glyph->advance[0], glyph->advance[1], 0.);
	  break;
	}
      }
    }
  }

  if (!node) {
    __GLCcharacter prevCode = {0, NULL, 0, {0, 0}};

    __glcProcessChar(ctx, code, &prevCode, GL_FALSE, __glcRenderChar, NULL);
  }

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, ctx, GL_FALSE);
  if (listIndex)
    ctx->enableState.glObjects = saveGLObjects;
}



/* This internal function is used by both glcRenderString() and
 * glcRenderCountedString(). The string 'inString' must be sorted in visual
 * order and stored using UCS4 format.
 */
static void __glcRenderCountedString(__GLCcontext* inContext, GLCchar* inString,
				     GLboolean inIsRightToLeft,GLint inCount)
{
  GLint listIndex = 0;
  GLint i = 0;
  GLCchar32* ptr = NULL;
  __GLCglState GLState;
  __GLCcharacter prevCode = {0, NULL, 0, {0, 0}};
  GLboolean saveGLObjects = GL_FALSE;
  GLint shift = 1;
  __GLCcharacter* chars = NULL;

  /* Disable the internal management of GL objects when the user is currently
   * building a display list.
   */
  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  if (listIndex) {
    saveGLObjects = inContext->enableState.glObjects;
    inContext->enableState.glObjects = GL_FALSE;
  }

  if (inContext->enableState.glObjects
      && inContext->renderState.renderStyle != GLC_BITMAP) {
    chars = (__GLCcharacter*)__glcMalloc(inCount * sizeof(__GLCcharacter));
    if (!chars) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }

  /* Save the value of the GL parameters */
  __glcSaveGLState(&GLState, inContext, GL_FALSE);

  if (inContext->renderState.renderStyle == GLC_LINE ||
      inContext->renderState.renderStyle == GLC_TRIANGLE) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }

  /* Set the texture environment if the render style is GLC_TEXTURE */
  if (inContext->renderState.renderStyle == GLC_TEXTURE) {
    /* Set the new values of the parameters */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (inContext->enableState.glObjects && inContext->atlas.id) {
      glBindTexture(GL_TEXTURE_2D, inContext->atlas.id);
      if (GLEW_ARB_vertex_buffer_object && inContext->atlas.bufferObjectID) {
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, inContext->atlas.bufferObjectID);
	glInterleavedArrays(GL_T2F_V3F, 0, NULL);
      }
    }
    else if (!inContext->enableState.glObjects && inContext->texture.id) {
      glBindTexture(GL_TEXTURE_2D, inContext->texture.id);
      if (GLEW_ARB_pixel_buffer_object && inContext->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			inContext->texture.bufferObjectID);
    }
  }

  /* Render the string */
  ptr = inString;
  if (inIsRightToLeft) {
    ptr += inCount - 1;
    shift = -1;
  }

  if (inContext->enableState.glObjects
      && inContext->renderState.renderStyle != GLC_BITMAP) {
    __GLCfont* font = NULL;
    __GLCglyph* glyph = NULL;
    int length = 0;
    int j = 0;
    GLuint DLindex = inContext->renderState.renderStyle - 0x101;
    FT_ListNode node = NULL;

    if (inContext->renderState.renderStyle == GLC_TRIANGLE
	&& inContext->enableState.extrude)
      DLindex++;

    for (i = 0; i < inCount; i++) {
      if (*ptr >= 32) {
 	for (node = inContext->currentFontList.head; node ; node = node->next) {
 	  font = (__GLCfont*)node->data;
 	  glyph = __glcCharMapGetGlyph(font->charMap, *ptr);

 	  if (glyph) {
 	    if (!glyph->displayList[DLindex] && !glyph->isSpacingChar)
 	      continue;

 	    if (inContext->renderState.renderStyle == GLC_TEXTURE
		&& !glyph->isSpacingChar) {
 	      FT_List_Up(&inContext->atlasList,
 			 (FT_ListNode)glyph->textureObject);
	      if (GLEW_ARB_vertex_buffer_object)
		chars[length].glObjectID =
		  ((__GLCatlasElement*)glyph->textureObject)->position * 4;
	      else
		chars[length].glObjectID = glyph->displayList[DLindex];
	    }
	    else
	      chars[length].glObjectID = glyph->displayList[DLindex];

 	    chars[length].advance[0] = glyph->advance[0];
 	    chars[length].advance[1] = glyph->advance[1];

 	    if (inContext->enableState.kerning) {
 	      __GLCcharacter* previous = NULL;

 	      if (length)
 		previous = &chars[length-1];
 	      else
 		previous = &prevCode;

 	      if (previous->code && previous->font == font) {
 		GLfloat kerning[2];
 		GLint leftCode = inIsRightToLeft ? *ptr : previous->code;
 		GLint rightCode = inIsRightToLeft ? previous->code : *ptr;

 		if (__glcFontGetKerning(font, leftCode, rightCode, kerning,
 					inContext, GLC_POINT_SIZE,
 					GLC_POINT_SIZE)) {
 		  if (inIsRightToLeft)
 		    chars[length].advance[0] -= kerning[0];
 		  else
 		    chars[length].advance[0] += kerning[0];

 		  chars[length].advance[1] += kerning[1];
 		}
 	      }
 	      chars[length].font = font;
 	    }

	    if (glyph->isSpacingChar)
	      chars[length].code = 32;
	    else
	      chars[length].code = *ptr;

 	    length++;
 	    break;
 	  }
 	}
      }

      if(!node || (i == inCount-1)) {
	for (j = 0; j < length; j++) {
	  if (inIsRightToLeft)
	    glTranslatef(-chars[j].advance[0], chars[j].advance[1], 0.);
	  if (chars[j].code != 32) {
	    if ((inContext->renderState.renderStyle == GLC_TEXTURE)
		&& GLEW_ARB_vertex_buffer_object)
	      glDrawArrays(GL_QUADS, chars[j].glObjectID, 4);
	    else
	      glCallList(chars[j].glObjectID);
	  }
	  if (!inIsRightToLeft)
	    glTranslatef(chars[j].advance[0], chars[j].advance[1], 0.);
	}
	if (length) {
	  prevCode.code = chars[length-1].code;
	  prevCode.font = chars[length-1].font;
	}
	if (!node)
	  __glcProcessChar(inContext, *ptr, &prevCode, inIsRightToLeft,
			   __glcRenderChar, NULL);
	length = 0;
      }

      ptr += shift;
    }
  }
  else {
    for (i = 0; i < inCount; i++) {
      if (*ptr >= 32)
	__glcProcessChar(inContext, *(ptr), &prevCode, inIsRightToLeft,
			 __glcRenderChar, NULL);
      ptr += shift;
    }
  }

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, inContext, GL_FALSE);
  if (inContext->enableState.glObjects)
    __glcFree(chars);
  if (listIndex)
    inContext->enableState.glObjects = saveGLObjects;
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
  __GLCcontext *ctx = NULL;
  GLCchar32* UinString = NULL;
  GLboolean isRightToLeft = GL_FALSE;

  GLC_INIT_THREAD();

  /* Check if inCount is positive */
  if (inCount < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
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

  __glcRenderCountedString(ctx, UinString, isRightToLeft, inCount);
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
  GLCchar32* UinString = NULL;
  GLboolean isRightToLeft = GL_FALSE;
  GLint length = 0;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
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
  UinString = __glcConvertToVisualUcs4(ctx, &isRightToLeft, &length, inString);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  __glcRenderCountedString(ctx, UinString, isRightToLeft, length);
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

  GLC_INIT_THREAD();

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
  ctx = GLC_GET_CURRENT_CONTEXT();
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
  __GLCcontext *ctx = NULL;
  GLint code = 0;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
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
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  ctx->renderState.resolution = inVal;

  return;
}
