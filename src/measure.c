/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

/* This file defines the so-called "Measurement commands" described in chapter
 * 3.10 of the GLC specs
 */

#include <string.h>
#include <stdlib.h>

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

  /* Look for the font identified by 'inFont' in the GLC_FONT_LIST */
  for (node = inState->fontList->head; node; node = node->next) {
    font = (__glcFont*)node->data;
    if (font->id == inFont) break;
  }

  /* No font has been found */
  if (!node) return NULL;

  face = font->face;
  metrics = face->glyph->metrics;

  /* Retrieve which is the glyph that inCode is mapped to */
  /* TODO : use a dichotomic algo. instead*/
  for (i = 0; i < font->charMapCount; i++) {
    if ((FT_ULong)inCode == font->charMap[0][i]) {
      inCode = font->charMap[1][i];
      break;
    }
  }

  if (FT_Load_Char(face, inCode, FT_LOAD_NO_SCALE)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  switch(inMetric) {
  case GLC_BASELINE:
    outVec[0] = 0.;
    outVec[1] = 0.;
    outVec[2] = metrics.horiAdvance;
    outVec[3] = metrics.vertAdvance;
    if (inState->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], inState);
    return outVec;
  case GLC_BOUNDS:
    FT_Get_Glyph(face->glyph, &glyph);
    FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &boundBox);
    outVec[0] = boundBox.xMin;
    outVec[1] = boundBox.yMin;
    outVec[2] = boundBox.xMax;
    outVec[3] = outVec[1];
    outVec[4] = outVec[2];
    outVec[5] = boundBox.yMax;
    outVec[6] = outVec[0];
    outVec[7] = outVec[5];
    if (inState->renderStyle == GLC_BITMAP) {
      __glcTransformVector(&outVec[0], inState);
      __glcTransformVector(&outVec[2], inState);
      __glcTransformVector(&outVec[4], inState);
      __glcTransformVector(&outVec[6], inState);
    }
    FT_Done_Glyph(glyph);
    return outVec;
  }

  return NULL;
}


/* glcGetCharMetric:
 *   This command is identical to the command glcRenderChar, except that
 *   instead of rendering the character tha inCode is mapped to, the command
 *   measures the resulting layout and stores in outVec the value of the
 *   metric identified by inMetric. If the command does not raise an error,
 *   its return value is outVec.
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
  repCode = glcGeti(GLC_REPLACEMENT_CODE);
  font = __glcCtxGetFont(state, repCode);
  if (repCode && font)
    return __glcGetCharMetric(repCode, inMetric, outVec, font, state);

  /* Here GLC must render /<hexcode>. Should we return the metrics of
     the whole string or nothing at all ? */
  return NULL;
}



/* glcGetMaxCharMetric:
 *   This command measures the layout that would result from rendering all
 *   mapped characters at the same origin.
 *   The command stores in outVec the value of the metric identified by
 *   inMetric. If the command does not raise an error, its return value
 *   is outVec.
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
  for (node = state->currentFontList->head; node; node = node->next) {
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
    outVec[2] = advance;
    outVec[3] = 0.;
    if (state->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], state);
    return outVec;
  case GLC_BOUNDS:
    outVec[0] = xl;
    outVec[1] = yb;
    outVec[2] = xr;
    outVec[3] = outVec[1];
    outVec[4] = outVec[2];
    outVec[5] = yt;
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



/* glcGetStringCharMetric:
 *   This command retrieves a character metric from the GLC measurement buffer
 *   and stores it in outVec. The character is identified by inIndex, and the
 *   metric is identified by inMetric. The command raises GLC_PARAMETER_ERROR
 *   if inIndex is less than zero or is greater than or equal to the value of
 *   the variable GLC_MEASURED_CHAR_COUNT. If the command does not raise an
 *   error, its return value is outVec.
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
    outVec[0] = state->measurementCharBuffer[inIndex][0];
    outVec[1] = state->measurementCharBuffer[inIndex][1];
    outVec[2] = state->measurementCharBuffer[inIndex][2];
    outVec[3] = state->measurementCharBuffer[inIndex][3];
    if (state->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], state);
    return outVec;
  case GLC_BOUNDS:
    outVec[0] = state->measurementCharBuffer[inIndex][4];
    outVec[1] = state->measurementCharBuffer[inIndex][5];
    outVec[2] = state->measurementCharBuffer[inIndex][6];
    outVec[3] = state->measurementCharBuffer[inIndex][7];
    outVec[4] = state->measurementCharBuffer[inIndex][8];
    outVec[5] = state->measurementCharBuffer[inIndex][9];
    outVec[6] = state->measurementCharBuffer[inIndex][10];
    outVec[7] = state->measurementCharBuffer[inIndex][11];
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



/* glcGetStringMetric:
 *   This command retrieves a string metric from the GLC measurement buffer
 *   and stores it in outVec. The metric is identified by inMatric. If the
 *   command does not raise an error, its return value is outVec.
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
    outVec[0] = state->measurementStringBuffer[0];
    outVec[1] = state->measurementStringBuffer[1];
    outVec[2] = state->measurementStringBuffer[2];
    outVec[3] = state->measurementStringBuffer[3];
    if (state->renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], state);
    return outVec;
  case GLC_BOUNDS:
    outVec[0] = state->measurementStringBuffer[4];
    outVec[1] = state->measurementStringBuffer[5];
    outVec[2] = state->measurementStringBuffer[6];
    outVec[3] = state->measurementStringBuffer[7];
    outVec[4] = state->measurementStringBuffer[8];
    outVec[5] = state->measurementStringBuffer[9];
    outVec[6] = state->measurementStringBuffer[10];
    outVec[7] = state->measurementStringBuffer[11];
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



/* glcMeasureCountedString:
 *   This command is identical to the command glcRenderCountedString, except
 *   that instead of rendering a string, the command measures the resulting
 *   layout and stores the measurement in the GLC measurement buffer. The
 *   string comprises the first inCount elements of the array inString, which
 *   need not be followed by a zero element. The command raises
 *   GLC_PARAMETER_ERROR if inCount is less than zero.
 *
 *   If the value inMeasureChars is nonzero, the command computes metrics for
 *   each character and for the overall string, and it assigns the value
 *   inCount to the variable GLC_MEASURED_CHARACTER_COUNT. Otherwise, the
 *   command computes metrics only for the overall string, and it assigns the
 *   value zero to the variable GLC_MEASURED_CHARACTER_COUNT.
 *
 *   If the command does not raise an error, its return value is the value of
 *   the variable GLC_MEASURED_CHARACTER_COUNT.
 */
GLint glcMeasureCountedString(GLboolean inMeasureChars, GLint inCount,
			      const GLCchar* inString)
{
  __glcContextState *state = NULL;
  GLint i = 0, j = 0;
  GLfloat baselineMetric[4] = {0., 0., 0., 0.};
  GLfloat boundsMetric[8] = {0., 0., 0., 0., 0., 0., 0., 0.};
  GLfloat storeBitmapMatrix[4] = {0., 0., 0., 0.};
  __glcUniChar UinString;

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

  UinString.ptr = (GLCchar*)inString;
  UinString.type = state->stringType;

  if (state->renderStyle == GLC_BITMAP) {
    for (i = 0; i < 4; i++)
      storeBitmapMatrix[i] = state->bitmapMatrix[i];
    glcLoadIdentity();
  }

  for (i = 0; i < 12; i++)
    state->measurementStringBuffer[i] = 0.;

  /* FIXME :
   * Computations performed below assume that the string is rendered
   * horizontally from left to right.
   */
  for (i = 0; i < inCount; i++) {
    /* For each character get the metrics */
    glcGetCharMetric(__glcUniIndex(&UinString, i), GLC_BASELINE,
		     baselineMetric);
    glcGetCharMetric(__glcUniIndex(&UinString, i), GLC_BOUNDS, boundsMetric);

    /* If characters are to be measured then store the results */
    if (inMeasureChars) {
      for (j = 0; j < 4; j++)
	state->measurementCharBuffer[i][j] = baselineMetric[j];
      for (j = 0; j < 8; j++)
	state->measurementCharBuffer[i][j + 4] = boundsMetric[j];
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

  if (state->renderStyle == GLC_BITMAP) {
    for (i = 0; i < 4; i++)
      state->bitmapMatrix[i] = storeBitmapMatrix[i];
    __glcTransformVector(&state->measurementStringBuffer[2], state);
    __glcTransformVector(&state->measurementStringBuffer[4], state);
    __glcTransformVector(&state->measurementStringBuffer[6], state);
    __glcTransformVector(&state->measurementStringBuffer[8], state);
    __glcTransformVector(&state->measurementStringBuffer[10], state);
    if (inMeasureChars)
      for (i = 0; i < inCount; i++) {
	__glcTransformVector(&state->measurementCharBuffer[i][2], state);
	__glcTransformVector(&state->measurementCharBuffer[i][4], state);
	__glcTransformVector(&state->measurementCharBuffer[i][6], state);
	__glcTransformVector(&state->measurementCharBuffer[i][8], state);
	__glcTransformVector(&state->measurementCharBuffer[i][10], state);
      }
  }

  if (inMeasureChars)
    state->measuredCharCount = inCount;
  else
    state->measuredCharCount = 0;

  return inCount;
}



/* glcMeasureString:
 *   This command is indetical to the command glcMeasureCountedString, except
 *   that inString is zero terminated, not counted.
 */
GLint glcMeasureString(GLboolean inMeasureChars, const GLCchar* inString)
{
  __glcContextState *state = NULL;
  __glcUniChar UinString;

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  UinString.ptr = (GLCchar*)inString;
  UinString.type = state->stringType;

  return glcMeasureCountedString(inMeasureChars, __glcUniLen(&UinString),
				 inString);
}
