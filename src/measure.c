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

/* \file
 * defines the so-called "Measurement commands" described in chapter 3.10 of
 * the GLC specs.
 */

/** \defgroup measure Measurement commands
 *  Those commands returns metrics (bounding box, baseline) of character or
 *  string layouts. Glyphs coordinates are defined in <em>em units</em> and are
 *  transformed during rendering to produce the desired mapping of the glyph
 *  shape into the GL window coordinate system. Moreover, GLC can return some
 *  metrics for a character and string layouts. The table below lists the
 *  metrics that are available :
 *  <center>
 *  <table>
 *  <caption>Metrics for character and string layout</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td> <td>Vector</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_BASELINE</b></td> <td>0x0030</td>
 *      <td>[ x<sub>l</sub> y<sub>l</sub> x<sub>r</sub> y<sub>r</sub> ]</td>
 *    </tr>
 *    <tr>
 *     <td><b>GLC_BOUNDS</b></td> <td>0x0031</td>
 *     <td>[ x<sub>lb</sub> y<sub>lb</sub> x<sub>rb</sub> y<sub>rb</sub>
 *           x<sub>rt</sub> y<sub>rt</sub> x<sub>lt</sub> y<sub>lt</sub> ]</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \n \b GLC_BASELINE is the line segment from the origin of the layout to the
 *  origin of the following layout. \b GLC_BOUNDS is the bounding box of the
 *  layout.
 *  \image html measure.png
 *  \n Each point <em>(x,y)</em> is computed in em coordinates, with the origin
 *  of a layout at <em>(0,0)</em>. If the value of the variable
 *  \b GLC_RENDER_STYLE is \b GLC_BITMAP, each point is transformed by the 2x2
 *  \b GLC_BITMAP_MATRIX.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_GLYPH_H



/* Multiply a vector by the GLC_BITMAP_MATRIX */
static void __glcTransformVector(GLfloat* outVec, __glcContextState *inState)
{
  GLfloat* matrix = inState->bitmapMatrix;
  GLfloat temp = 0.f;

  temp = matrix[0] * outVec[0] + matrix[2] * outVec[1];
  outVec[1] = matrix[1] * outVec[0] + matrix[3] * outVec[1];
  outVec[0] = temp;
}



/* Retrieve the metrics of a character identified by 'inCode' in a font
 * idetified by 'inFont'.
 */
static GLfloat* __glcGetCharMetric(GLint inCode, GLCenum inMetric,
				   GLfloat *outVec, GLint inFont,
				   __glcContextState *inState)
{
  FT_Face face = NULL;
  FT_Glyph_Metrics metrics;
  FT_Glyph glyph = NULL;
  FT_BBox boundBox;
  GLint i = 0;
  __glcFont *font = NULL;
  FT_ListNode node = NULL;
  FT_UInt glyphIndex = 0;

  /* Look for the font identified by 'inFont' in the GLC_FONT_LIST */
  for (node = inState->fontList.head; node; node = node->next) {
    font = (__glcFont*)node->data;
    if (font->id == inFont) break;
  }

  assert(node);

  face = font->face;
  metrics = face->glyph->metrics;

  /* Retrieve which is the glyph that inCode is mapped to */
  /* TODO : use a dichotomic algo. instead */
  for (i = 0; i < font->charMapCount; i++) {
    if ((FT_ULong)inCode == font->charMap[0][i]) {
      inCode = font->charMap[1][i];
      break;
    }
  }

  assert(FcCharSetHasChar(font->faceDesc->charSet, inCode));

  /* Get and load the glyph which unicode code is identified by inCode */
  glyphIndex = FcFreeTypeCharIndex(face, inCode);

  if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE |
		    FT_LOAD_IGNORE_TRANSFORM)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  FT_Get_Glyph(face->glyph, &glyph);

  switch(inMetric) {
  case GLC_BASELINE:
    outVec[0] = 0.;
    outVec[1] = 0.;
    outVec[2] = (GLfloat) face->glyph->advance.x / face->units_per_EM;
    outVec[3] = (GLfloat) face->glyph->advance.y / face->units_per_EM;
    if (inState->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], inState);
    break;
  case GLC_BOUNDS:
    FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &boundBox);
    outVec[0] = (GLfloat) boundBox.xMin / face->units_per_EM;
    outVec[1] = (GLfloat) boundBox.yMin / face->units_per_EM;
    outVec[2] = (GLfloat) boundBox.xMax / face->units_per_EM;
    outVec[3] = outVec[1];
    outVec[4] = outVec[2];
    outVec[5] = (GLfloat) boundBox.yMax / face->units_per_EM;
    outVec[6] = outVec[0];
    outVec[7] = outVec[5];
    if (inState->renderStyle == GLC_BITMAP) {
      for (i = 0; i < 4; i++)
	__glcTransformVector(&outVec[2*i], inState);
    }
    break;
  default:
    return NULL;
  }

  FT_Done_Glyph(glyph);
  return outVec;
}


/** \ingroup measure
 *  This command is identical to the command glcRenderChar(), except that
 *  instead of rendering the character that \e inCode is mapped to, the command
 *  measures the resulting layout and stores in \e outVec the value of the
 *  metric identified by \e inMetric. If the command does not raise an error,
 *  its return value is \e outVec.
 *  \param inCode The character to measure.
 *  \param inMetric The metric to measure, either \b GLC_BASELINE or
 *                   \b GLC_BOUNDS.
 *  \param outVec A vector in which to store value of \e inMetric for specified
 *                 character.
 *  \returns \e outVec if the command succeeds, \b NULL otherwise.
 *  \sa glcGetMaxCharMetric()
 *  \sa glcGetStringCharMetric()
 *  \sa glcMeasureCountedString()
 *  \sa glcMeasureString()
 */
GLfloat* glcGetCharMetric(GLint inCode, GLCenum inMetric, GLfloat *outVec)
{
  GLint repCode = 0;
  GLint font = 0;
  __glcContextState *state = NULL;

  switch(inMetric) {
  case GLC_BASELINE:
  case GLC_BOUNDS:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Finds a font that owns a glyph which maps to inCode */
  font = __glcCtxGetFont(state, inCode);
  if (font)
    return __glcGetCharMetric(inCode, inMetric, outVec, font, state);

  /* No glyph maps to in inCode. Use the replacement code instead */
  repCode = state->replacementCode;
  font = __glcCtxGetFont(state, repCode);
  if (repCode && font)
    return __glcGetCharMetric(repCode, inMetric, outVec, font, state);

  /* Here GLC must render /<hexcode>. Should we return the metrics of
     the whole string or nothing at all ? */
  return NULL;
}



/** \ingroup measure
 *  This command measures the layout that would result from rendering all
 *  mapped characters at the same origin. This contrast with
 *  glcGetStringCharMetric(), which measures characters as  part of a string,
 *  that is, influenced by kerning, ligatures, and so on.
 *
 *  The command stores in \e outVec the value of the metric identified by
 *  \e inMetric. If the command does not raise an error, its return value
 *  is \e outVec.
 *  \param inMetric The metric to measure, either \b GLC_BASELINE or
 *                  \b GLC_BOUNDS.
 *  \param outVec A vector in which to store value of \e inMetric for all
 *                mapped character.
 *  \returns \e outVec if the command succeeds, \b NULL otherwise.
 *  \sa glcGetCharMetric()
 *  \sa glcGetStringCharMetric()
 *  \sa glcMeasureCountedString()
 *  \sa glcMeasureString()
 */
GLfloat* glcGetMaxCharMetric(GLCenum inMetric, GLfloat *outVec)
{
  __glcContextState *state = NULL;
  FT_Face face = NULL;
  GLfloat advance = 0., yb = 0., yt = 0., xr = 0., xl = 0.;
  FT_ListNode node = NULL;

  /* Check the parameters */
  switch(inMetric) {
  case GLC_BASELINE:
  case GLC_BOUNDS:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* For each in GLC_CURRENT_FONT_LIST find the maximum values of the advance
   * width of the bounding boxes.
   */
  for (node = state->currentFontList.head; node; node = node->next) {
    GLfloat temp = 0.f;
    face = ((__glcFont*)node->data)->face;

    temp = (GLfloat)face->max_advance_width;
    if (temp > advance)
      advance = temp;

    temp = (GLfloat)face->bbox.yMax;
    if (temp > yt)
      yt = temp;

    temp = (GLfloat)face->bbox.yMin;
    if (temp > yb)
      yb = temp;

    temp = (GLfloat)face->bbox.xMax;
    if (temp > xr)
      xr = temp;

    temp = (GLfloat)face->bbox.xMin;
    if (temp > xl)
      xl = temp;
  }

  /* Update and transform, if necessary, the return value */
  switch(inMetric) {
  case GLC_BASELINE:
    outVec[0] = 0.;
    outVec[1] = 0.;
    outVec[2] = advance / face->units_per_EM;
    outVec[3] = 0.;
    if (state->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], state);
    return outVec;
  case GLC_BOUNDS:
    outVec[0] = xl / face->units_per_EM;
    outVec[1] = yb / face->units_per_EM;
    outVec[2] = xr / face->units_per_EM;;
    outVec[3] = outVec[1];
    outVec[4] = outVec[2];
    outVec[5] = yt / face->units_per_EM;
    outVec[6] = outVec[0];
    outVec[7] = outVec[5];
    if (state->renderStyle == GLC_BITMAP) {
      __glcTransformVector(&outVec[0], state);
      __glcTransformVector(&outVec[2], state);
      __glcTransformVector(&outVec[4], state);
      __glcTransformVector(&outVec[6], state);
    }
    return outVec;
  }

  return NULL;
}



/** \ingroup measure
 *  This command retrieves a character metric from the GLC measurement buffer
 *  and stores it in \e outVec. To store a string in the measurement buffer,
 *  call glcMeasureCountedString() or glcMeasureString().
 *
 *  The character is identified by \e inIndex, and the metric is identified by
 *  \e inMetric.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inIndex is less than zero
 *  or is greater than or equal to the value of the variable
 *  \b GLC_MEASURED_CHAR_COUNT. If the command does not raise an error, its
 *  return value is outVec.
 *  \par Example:
 *  The following example first calls glcMeasureString() to store the string
 *  "hello" in the measurement buffer. It then retrieves both the baseline and
 *  the bounding box for the whole string, then for each individual character.
 *
 *  \code
 *  GLfloat overallBaseline[4];
 *  GLfloat overallBoundingBox[8];
 *
 *  GLfloat charBaslines[4][5];
 *  GLfloat charBoundingBoxes[8][5];
 *
 *  GLint i;
 *
 *  glcMeasureString(GL_TRUE, "hello");
 *
 *  glcGetStringMetric(GLC_BASELINE, overallBaseline);
 *  glcGetStringMetric(GLC_BOUNDS, overallBoundingBox);
 *
 *  for (i = 0; i < 5; i++) {
 *      glcGetStringCharMetric(i, GLC_BASELINE, charBaselines[i]);
 *      glcGetStringCharMetric(i, GLC_BOUNDS, charBoundingBoxes[i]);
 *  }
 *  \endcode
 *  \note
 *  \e glcGetStringCharMetric is useful if you're interested in the metrics of
 *  a character as it appears in a string, that is, influenced by kerning,
 *  ligatures, and so on. To measure a character as if it started at the
 *  origin, call glcGetCharMetric().
 *  \param inIndex Specifies which element in the string to measure.
 *  \param inMetric The metric to measure, either \b GLC_BASELINE or
 *                  \b GLC_BOUNDS.
 *  \param outVec A vector in which to store value of \e inMetric for the
 *                character identified by \e inIndex.
 *  \returns \e outVec if the command succeeds, \b NULL otherwise.
 *  \sa glcGetCharMetric()
 *  \sa glcGetMaxCharMetric()
 *  \sa glcMeasureCountedString()
 *  \sa glcMeasureString()
 */
GLfloat* glcGetStringCharMetric(GLint inIndex, GLCenum inMetric,
				GLfloat *outVec)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inMetric) {
  case GLC_BASELINE:
  case GLC_BOUNDS:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify that inIndex is in legal bounds */
  if ((inIndex < 0) || (inIndex >= state->measuredCharCount)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  switch(inMetric) {
  case GLC_BASELINE:
    memcpy(outVec, state->measurementCharBuffer[inIndex], 4*sizeof(GLfloat));
    return outVec;
  case GLC_BOUNDS:
    memcpy(outVec, &state->measurementCharBuffer[inIndex][4],
	   8*sizeof(GLfloat));
    return outVec;
  }

  return NULL;
}



/** \ingroup measure
 *  This command retrieves a string metric from the GLC measurement buffer
 *  and stores it in \e outVec. The metric is identified by \e inMetric. To
 *  store a string from the GLC measurement buffer, call
 *  glcMeasureCountedString() or glcMeasureString().
 *
 *  If the command does not raise an error, its return value is \e outVec.
 *  \param inMetric The metric to measure, either \b GLC_BASELINE or
 *         \b GLC_BOUNDS.
 *  \param outVec A vector in which to store value of \e inMetric for the
 *                character identified by \e inIndex.
 *  \returns \e outVec if the command succeeds, \b NULL otherwise.
 *  \sa glcGetCharMetric()
 *  \sa glcGetMaxCharMetric()
 *  \sa glcGetStringCharMetric()
 *  \sa glcMeasureCountedString()
 *  \sa glcMeasureString()
 */
GLfloat* glcGetStringMetric(GLCenum inMetric, GLfloat *outVec)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inMetric) {
  case GLC_BASELINE:
  case GLC_BOUNDS:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  switch(inMetric) {
  case GLC_BASELINE:
    memcpy(outVec, state->measurementStringBuffer, 4*sizeof(GLfloat));
    return outVec;
  case GLC_BOUNDS:
    memcpy(outVec, &state->measurementStringBuffer[4], 8*sizeof(GLfloat));
    return outVec;
  }

  return NULL;
}



static GLint __glcMeasureCountedString(__glcContextState *state,
				GLboolean inMeasureChars, GLint inCount,
				const FcChar8* inString)
{
  GLint i = 0, j = 0;
  GLfloat baselineMetric[4] = {0., 0., 0., 0.};
  GLfloat boundsMetric[8] = {0., 0., 0., 0., 0., 0., 0., 0.};
  const FcChar8* ptr = NULL;
  const GLint storeRenderStyle = state->renderStyle;

  if (state->renderStyle == GLC_BITMAP)
    state->renderStyle = 0;

  memset(state->measurementStringBuffer, 0, 12*sizeof(GLfloat));

  /* FIXME :
   * Computations performed below assume that the string is rendered
   * horizontally from left to right.
   */
  ptr = inString;
  for (i = 0; i < inCount; i++) {
    FcChar32 code = 0;
    int len = 0;

    len = FcUtf8ToUcs4(ptr, &code, strlen((const char*)ptr));
    if (len < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    ptr += len;
    /* For each character get the metrics */
    glcGetCharMetric(code, GLC_BASELINE, baselineMetric);
    glcGetCharMetric(code, GLC_BOUNDS, boundsMetric);

    /* If characters are to be measured then store the results */
    if (inMeasureChars) {
      memcpy(state->measurementCharBuffer[i], baselineMetric,
	     4*sizeof(GLfloat));
      memcpy(&state->measurementCharBuffer[i][j+4], boundsMetric,
	     8*sizeof(GLfloat));
    }

    /* Initialize the left-most coordinate of the bounding box */
    if (!i) {
      state->measurementStringBuffer[4] = boundsMetric[0];
      state->measurementStringBuffer[10] = boundsMetric[0];
    }

    /* Compute string advance */
    state->measurementStringBuffer[2] += baselineMetric[2];

    /* Compute the bottom value of the string */
    if (state->measurementStringBuffer[5] > boundsMetric[1]) {
      state->measurementStringBuffer[5] = boundsMetric[1];
      state->measurementStringBuffer[7] = boundsMetric[1];
    }

    /* Compute the top value of the string */
    if (state->measurementStringBuffer[9] < boundsMetric[5]) {
      state->measurementStringBuffer[9] = boundsMetric[5];
      state->measurementStringBuffer[11] = boundsMetric[5];
    }

    /* Add the advance to the right-most value of the bounding box in
       order to take bounding box width *and* space between characters
       into account */
    state->measurementStringBuffer[6] += baselineMetric[2];
    state->measurementStringBuffer[8] += baselineMetric[2];
  }

  /* Correction to the right-most value : since every character have been
     taken into account, we do not need to take the space after the last
     bounding box into account */
  state->measurementStringBuffer[6] -= (baselineMetric[2] - boundsMetric[4]);
  state->measurementStringBuffer[8] -= (baselineMetric[2] - boundsMetric[4]);

  if (storeRenderStyle == GLC_BITMAP) {
    state->renderStyle = storeRenderStyle;
    for (i = 0; i < 6; i++)
      __glcTransformVector(&state->measurementStringBuffer[2*i], state);
    if (inMeasureChars) {
      int j = 0;
      for (i = 0; i < inCount; i++) {
	for (j = 0; j < 6; j++)
	  __glcTransformVector(&state->measurementCharBuffer[i][2*j], state);
      }
    }
  }

  if (inMeasureChars)
    state->measuredCharCount = inCount;
  else
    state->measuredCharCount = 0;

  return inCount;
}



/** \ingroup measure
 *  This command is identical to the command glcRenderCountedString(), except
 *  that instead of rendering a string, the command measures the resulting
 *  layout and stores the measurement in the GLC measurement buffer. The
 *  string comprises the first \e inCount elements of the array \e inString,
 *  which need not be followed by a zero element. 
 *
 *  If the value \e inMeasureChars is nonzero, the command computes metrics for
 *  each character and for the overall string, and it assigns the value
 *  \e inCount to the variable \b GLC_MEASURED_CHARACTER_COUNT. Otherwise, the
 *  command computes metrics only for the overall string, and it assigns the
 *  value zero to the variable \b GLC_MEASURED_CHARACTER_COUNT.
 *
 *  If the command does not raise an error, its return value is the value of
 *  the variable \b GLC_MEASURED_CHARACTER_COUNT.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inCount is less than zero.
 *  \param inMeasureChars Specifies whether to compute metrics only for the
 *                        string or for the characters as well.
 *  \param inCount The number of elements to measure, starting at the first
 *                 element.
 *  \param inString The string to be measured.
 *  \returns The variable \b GLC_MEASURED_CHARACTER_COUNT if the command
 *           succeeds, zero otherwise.
 *  \sa glcGeti() with argument GLC_MEASURED_CHAR_COUNT
 *  \sa glcGetStringCharMetric()
 *  \sa glcGetStringMetric()
 */
GLint glcMeasureCountedString(GLboolean inMeasureChars, GLint inCount,
			      const GLCchar* inString)
{
  __glcContextState *state = NULL;
  GLint count = 0;
  FcChar8* UinString = NULL;

  /* Check the parameters */
  if (inCount < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  UinString = __glcConvertCountedStringToUtf8(inCount, inString,
					      state->stringType);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  count = __glcMeasureCountedString(state, inMeasureChars, inCount, UinString);

  free(UinString);

  return count;
}



/** \ingroup measure
 *  This command measures the layout that would result from rendering a string
 *  and stores the measurements in the GLC measurement buffer. This command
 *  is identical to the command glcMeasureCountedString(), except that
 *  \e inString is zero terminated, not counted.
 *
 *  If the command does not raise an error, its return value is the value of
 *  the variable \b GLC_MEASURED_CHARACTER_COUNT.
 *  \param inMeasureChars Specifies whether to compute metrics only for the
 *                        string or for the characters as well.
 *  \param inString The string to be measured.
 *  \returns The variable \b GLC_MEASURED_CHARACTER_COUNT if the command
 *           succeeds, zero otherwise.
 *  \sa glcGeti() with argument GLC_MEASURED_CHAR_COUNT
 *  \sa glcGetStringCharMetric()
 *  \sa glcGetStringMetric()
 */
GLint glcMeasureString(GLboolean inMeasureChars, const GLCchar* inString)
{
  __glcContextState *state = NULL;
  FcChar8* UinString = NULL;
  GLint count = 0;
  GLint len = 0;
  int shift = 0;
  FcChar32 dummy = 0;
  FcChar8* utf8 = NULL;

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  UinString = __glcConvertToUtf8(inString, state->stringType);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  /* Compute the number of characters of the string */
  utf8 = UinString;
  len = 0;
  while(*utf8) {
    shift = FcUtf8ToUcs4(utf8, &dummy, 6);
    if (shift < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    utf8 += shift;
    len += shift;
  }

  count = __glcMeasureCountedString(state, inMeasureChars, len, UinString);

  free(UinString);

  return count;
}
