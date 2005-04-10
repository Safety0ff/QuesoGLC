/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_LIST_H

/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(__glcFont* inFont,
				  __glcContextState* inState)
{
  FT_Matrix matrix;
  FT_Face face = inFont->face;
  FT_Outline outline = face->glyph->outline;
  FT_BBox boundBox;
  FT_Bitmap pixmap;

  /* compute glyph dimensions */
  matrix.xx = (FT_Fixed)(inState->bitmapMatrix[0] * 65536. / GLC_POINT_SIZE);
  matrix.xy = (FT_Fixed)(inState->bitmapMatrix[2] * 65536. / GLC_POINT_SIZE);
  matrix.yx = (FT_Fixed)(inState->bitmapMatrix[1] * 65536. / GLC_POINT_SIZE);
  matrix.yy = (FT_Fixed)(inState->bitmapMatrix[3] * 65536. / GLC_POINT_SIZE);

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
}


static int __glcNextPowerOf2(int value)
{
  int power = 0;

  for (power = 1; power < value; power <<= 1);

  return power;
}

/* Internal function that renders glyph in textures */
static void __glcRenderCharTexture(__glcFont* inFont,
				   __glcContextState* inState, GLint inCode)
{
  FT_Face face = inFont->face;
  FT_Outline outline = face->glyph->outline;
  FT_BBox boundBox;
  FT_Bitmap pixmap;
  GLuint texture = 0;
  GLfloat width = 0, height = 0;
  GLint format = 0;
  __glcDisplayListKey *dlKey = NULL;
  FT_ListNode node = NULL;

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
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_LUMINANCE, pixmap.width,
	       pixmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
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
    return;
  }

  /* Create a texture object and make it current */
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  /* Add the new texture to the texture list */
  if (inState->glObjects) {
    FT_ListNode node = NULL;

    node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
    if (node) {
      node->data = (void*)texture;
      FT_List_Add(&inFont->parent->textureObjectList, node);
    }
    else
      __glcRaiseError(GLC_RESOURCE_ERROR);
  }

  /* Create the texture */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (inState->mipmap) {
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE8, pixmap.width,
		      pixmap.rows, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		      pixmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		    GL_LINEAR_MIPMAP_LINEAR);
  }
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, pixmap.width,
		 pixmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		 pixmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  if (inState->glObjects) {
    dlKey = (__glcDisplayListKey *)__glcMalloc(sizeof(__glcDisplayListKey));
    if (!dlKey) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(pixmap.buffer);
      return;
    }

    /* Initialize the key */
    dlKey->list = glGenLists(1);
    dlKey->faceDesc = inFont->faceDesc;
    dlKey->code = inCode;
    dlKey->renderMode = 2;

    /* Get (or create) a new entry that contains the display list and store
     * the key in it
     */
    node = (FT_ListNode)dlKey;
    node->data = dlKey;
    FT_List_Add(&inFont->parent->displayList, node);

    /* Create the display list */
    glNewList(dlKey->list, GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, texture);
  }

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

  /* Stores the glyph advance in the display list */
  glTranslatef(face->glyph->advance.x / 64. / GLC_POINT_SIZE, 
	       face->glyph->advance.y / 64. / GLC_POINT_SIZE, 0.);

  glBindTexture(GL_TEXTURE_2D, 0);

  if (inState->glObjects) {
    /* Finish display list creation */
    glEndList();
    glCallList(dlKey->list);
  }
  else
    glDeleteTextures(1, &texture);

  __glcFree(pixmap.buffer);
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
				      GLint renderMode)
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

/* Internal function that is called to do the actual rendering */
static void __glcRenderChar(GLint inCode, GLint inFont)
{
  __glcContextState *state = __glcGetCurrent();
  __glcFont* font = NULL;
  FT_UInt glyphIndex = 0;
  GLint i = 0;
  FT_ListNode node = NULL;

  for (node = state->fontList.head; node; node = node->next) {
    font = (__glcFont*)node->data;
    assert(font);
    if (font->id == inFont) break;
  }

  if (!node)
    return;

  assert(FcCharSetHasChar(font->faceDesc->charSet, inCode));

  /* Convert the code 'inCode' using the charmap */
  /* TODO : use a dichotomic algo. instead*/
  for (i = 0; i < font->charMapCount; i++) {
    if ((FT_ULong)inCode == font->charMap[0][i]) {
      inCode = (GLint)font->charMap[1][i];
      break;
    }
  }

  /* Define the size of the rendered glyphs (based on screen resolution) */
  if (FT_Set_Char_Size(font->face, GLC_POINT_SIZE << 6, 0,
		       (FT_UInt)state->resolution, (FT_UInt)state->resolution))
    { 
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

  /* Get and load the glyph which unicode code is identified by inCode */
  glyphIndex = FcFreeTypeCharIndex(font->face, inCode);

  if (FT_Load_Glyph(font->face, glyphIndex, FT_LOAD_NO_BITMAP |
		    FT_LOAD_IGNORE_TRANSFORM)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Call the appropriate function depending on the rendering mode. It first
   * checks if a display list that draws the desired glyph has already been
   * defined
   */
  switch(state->renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(font, state);
    return;
  case GLC_TEXTURE:
    if (!__glcFindDisplayList(font, inCode, 2))
      __glcRenderCharTexture(font, state, inCode);
    return;
  case GLC_LINE:
  case GLC_TRIANGLE:
    if (!__glcFindDisplayList(font, inCode,
			      (state->renderStyle == GLC_TRIANGLE) ? 4 : 3))
      __glcRenderCharScalable(font, state, inCode, 
			      (state->renderStyle == GLC_TRIANGLE));
    return;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }
}

/** \ingroup render
 *  This command renders the character that \e inCode is mapped to.
 *
 *  GLC finds a font that maps \e inCode to a character such as LATIN CAPITAL
 *  LETTER A, then uses one or more glyphs from the font to create a graphical
 *  layout that represents the character. Finally, GLC issues a sequence of GL
 *  commands to draw the layout. Glyph coordinates are defined in em units and
 *  are transformed during rendering to produce the desired mapping of the
 *  glyph shape into the GL window coordinate system.
 *
 *  If \e glcRenderChar cannot find a font in the list \b GLC_CURRENT_FONT_LIST
 *  that maps \e inCode, it attemps to produce an alternate rendering. If the
 *  value of the boolean variable \b GLC_AUTO_FONT is set to \b GL_TRUE,
 *  \b glcRenderChar finds a font that has the character that maps \e inCode.
 *  If the search succeeds, \e glcRenderChar appends the font's ID to
 *  \b GLC_CURRENT_FONT_LIST and renders the character.
 *
 *  If there are fonts in the list \b GLC_CURRENT_FONT_LIST, but a match for
 *  \e inCode cannot be found in any of those fonts, \e glcRenderChar goes
 *  through these steps :
 *  -# If the value of the variable \b GLC_REPLACEMENT_CODE is nonzero,
 *  \e glcRenderChar finds a font that maps the replacement code, and renders
 *  the character that the replacement code is mapped to.
 *  -# If the variable \b GLC_REPLACEMENT_CODE is zero, or if the replacement
 *  code does not result in a match, \e glcRenderChar checks whether a callback
 *  function is defined. If a callback function is defined for
 *  \b GLC_OP_glcUnmappedCode, \e glcRenderChar calls the function. The
 *  callback function provides \e inCode to the user and allows loading of the
 *  appropriate font. After the callback returns, \e glcRenderChar tries to
 *  render \e inCode again.
 *  -# Otherwise, the command attemps to render the character sequence
 *  <em>\\\<hexcode\></em>, where \\ is the character REVERSE SOLIDUS (U+5C),
 *  \< is the character LESS-THAN SIGN (U+3C), \> is the character GREATER-THAN
 *  SIGN (U+3E), and \e hexcode is \e inCode represented as a sequence of
 *  hexadecimal digits. The sequence has no leading zeros, and alphabetic
 *  digits are in upper case. The GLC measurement commands treat the sequence
 *  as a single character.
 *
 *  \param inCode The character to render
 *  \sa glcRenderString()
 *  \sa glcRenderCountedString()
 *  \sa glcReplacementCode()
 *  \sa glcRenderStyle()
 *  \sa glcCallbackFunc()
 */
void glcRenderChar(GLint inCode)
{
  GLint repCode = 0;
  GLint font = 0;
  __glcContextState *state = NULL;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get a font that maps inCode */
  font = __glcCtxGetFont(state, inCode);
  if (font) {
    /* A font has been found */
    __glcRenderChar(inCode, font);
    return;
  }

  /* __glcCtxGetFont() can not find a font that maps inCode, we then attempt to
   * produce an alternate rendering.
   */

  /* If the variable GLC_REPLACEMENT_CODE is nonzero, and __glcCtxGetFont()
   * finds a font that maps the replacement code, we now render the character
   * that the replacement code is mapped to
   */
  repCode = glcGeti(GLC_REPLACEMENT_CODE);
  font = __glcCtxGetFont(state, repCode);
  if (repCode && font) {
    __glcRenderChar(repCode, font);
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
    if (!__glcCtxGetFont(state, '\\') || !__glcCtxGetFont(state, '<')
	|| !__glcCtxGetFont(state, '>'))
      return;

    /* Check if a font maps hexadecimal digits */
    sprintf(buf,"%X", (int)inCode);
    n = strlen(buf);
    for (i = 0; i < n; i++) {
      if (!__glcCtxGetFont(state, buf[i]))
	return;
    }

    /* Render the '\<hexcode>' sequence */
    __glcRenderChar('\\', __glcCtxGetFont(state, '\\'));
    __glcRenderChar('<', __glcCtxGetFont(state, '<'));
    for (i = 0; i < n; i++)
      __glcRenderChar(buf[i], __glcCtxGetFont(state, buf[i]));
    __glcRenderChar('>', __glcCtxGetFont(state, '>'));
  }
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
    ptr += FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    glcRenderChar(code);
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
    ptr += FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    glcRenderChar(code);
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
 *  \param inCode An integer to assign to \b GLC_REPLACEMENT_CODE.
 *  \sa glcGeti() with argument \b GLC_REPLACEMENT_CODE
 *  \sa glcRenderChar()
 */
void glcReplacementCode(GLint inCode)
{
  __glcContextState *state = __glcGetCurrent();

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the replacement code */
  state->replacementCode = inCode;
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

  /* Check if the current thread owns a current state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  state->resolution = inVal;
  return;
}
