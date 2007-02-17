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
 *
 *  \image html measure.png "Baseline and bounds"
 *  \image latex measure.eps "Baseline and bounds" width=7cm
 *  \n Each point <em>(x,y)</em> is computed in em coordinates, with the origin
 *  of a layout at <em>(0,0)</em>. If the value of the variable
 *  \b GLC_RENDER_STYLE is \b GLC_BITMAP, each point is transformed by the 2x2
 *  \b GLC_BITMAP_MATRIX.
 */

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#define GLCAPI __declspec(dllexport)
#endif

#include "internal.h"
#include <math.h>
#include FT_GLYPH_H



/* Multiply a vector by the GLC_BITMAP_MATRIX */
static void __glcTransformVector(GLfloat* outVec, GLfloat *inMatrix)
{
  GLfloat temp = inMatrix[0] * outVec[0] + inMatrix[2] * outVec[1];

  outVec[1] = inMatrix[1] * outVec[0] + inMatrix[3] * outVec[1];
  outVec[0] = temp;
}



/* Retrieve the metrics of a character identified by 'inCode' in a font
 * identified by 'inFont'.
 * 'inCode' must be given in UCS-4 format
 */
static void* __glcGetCharMetric(GLint inCode, GLint inPrevCode, GLint inFont,
				__glcContextState* inState, void* inData,
				GLboolean inMultipleChars)
{
  GLfloat* outVec = (GLfloat*)inData;
  GLint i = 0;
  GLfloat xMin = 0., xMax = 0.;
  GLfloat yMin = 0., yMax = 0.;
  GLfloat scale_x = GLC_POINT_SIZE;
  GLfloat scale_y = GLC_POINT_SIZE;
  GLfloat transformMatrix[16];
  GLfloat temp[4];
  __glcFont* font = __glcVerifyFontParameters(inFont);

  /* Get the scale factors in X and Y direction from the transformation
   * matrix
   */
  __glcGetScale(inState, transformMatrix, &scale_x, &scale_y);

  /* Exit if the font is undefined or if one of the scale factors is almost
   * zero (that is the matrix transforms the glyph so that its height or length
   * is zero).
   */
  if ((!font) || (fabs(scale_x) < GLC_EPSILON)
      || (fabs(scale_y) < GLC_EPSILON))
    return NULL;

  if (inMultipleChars && (inState->renderState.renderStyle == GLC_BITMAP)) {
    /* If a string (or several characters) is to be measured, it will be easier
     * to perform the calculations in the glyph coordinate system than in the
     * screen coordinate system. In order to get the values that already stored
     * in outVec back in the glyph coordinate system, we must compute the
     * inverse of the transformation matrix.
     * Note that the inverse of the transformation exists since we have already
     * checked that the scale factors are not zero.
     */
    GLfloat* matrix = inState->bitmapMatrix;
    GLfloat inverseMatrix[4];
    GLfloat norm = 0.;
    GLfloat determinant = matrix[0] * matrix[3]	- matrix[1] * matrix[2];

    for (i = 0; i < 4; i++) {
      if (fabs(matrix[i] > norm))
	norm = fabs(matrix[i]);
    }

    if (determinant >= norm * GLC_EPSILON) {
      inverseMatrix[0] = matrix[3] / determinant;
      inverseMatrix[1] = -matrix[1] / determinant;
      inverseMatrix[2] = -matrix[2] / determinant;
      inverseMatrix[3] = matrix[0] / determinant;
    }
    else
      return NULL;

    /* Transform the values in outVec from the screen coordinate system to the
     * the glyph coordinate system
     */
    for (i = 0; i < 7; i++)
      __glcTransformVector(&outVec[2*i], inverseMatrix);
  }

  if (!inMultipleChars) {
      outVec[0] = 0.;
      outVec[1] = 0.;
      outVec[2] = 0.;
      outVec[3] = 0.;
  }
  else {
    outVec[2] -= outVec[12];
    outVec[3] -= outVec[13];
  }

  __glcFontGetBoundingBox(font, inCode, temp, inState, scale_x, scale_y);
  /* Take into account the advance of the glyphs that have already been
   * measured.
   */
  xMin = temp[0] + outVec[2];
  yMin = temp[1] + outVec[3];
  xMax = temp[2] + outVec[2];
  yMax = temp[3] + outVec[3];

  /* Update the global bounding box */
  if (inMultipleChars) {
    outVec[4] = xMin < outVec[4] ? xMin : outVec[4];
    outVec[5] = yMin < outVec[5] ? yMin : outVec[5];
    outVec[6] = xMax > outVec[6] ? xMax : outVec[6];
    outVec[9] = yMax > outVec[9] ? yMax : outVec[9];
  }
  else {
    outVec[4] = xMin;
    outVec[5] = yMin;
    outVec[6] = xMax;
    outVec[9] = yMax;
  }
  /* Finalize the update of the bounding box coordinates */
  outVec[7] = outVec[5];
  outVec[8] = outVec[6];
  outVec[10] = outVec[4];
  outVec[11] = outVec[9];

  /* Get the advance of the glyph */
  __glcFontGetAdvance(font, inCode, temp, inState, scale_x, scale_y);
  /* Update the global advance accordingly */
  outVec[2] += temp[0] / GLC_POINT_SIZE;
  outVec[3] += temp[1] / GLC_POINT_SIZE;

  outVec[12] = 0.;
  outVec[13] = 0.;
  if (inPrevCode && inState->enableState.kerning) {
    GLfloat kerning[2];

    if (__glcFontGetKerning(font, inCode, inPrevCode, kerning, inState, scale_x,
			    scale_y))
    {
      outVec[12] = kerning[0] / GLC_POINT_SIZE;
      outVec[13] = kerning[1] / GLC_POINT_SIZE;
    }
  }

  /* Transforms the values into the screen coordinate system if necessary */
  if (inState->renderState.renderStyle == GLC_BITMAP) {
    for (i = 0; i < 7; i++)
      __glcTransformVector(&outVec[2*i], inState->bitmapMatrix);
  }

  return outVec;
}



/** \ingroup measure
 *  This command is identical to the command glcRenderChar(), except that
 *  instead of rendering the character that \e inCode is mapped to, the command
 *  measures the resulting layout and stores in \e outVec the value of the
 *  metric identified by \e inMetric. If the command does not raise an error,
 *  its return value is \e outVec.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e outVec is NULL.
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
GLfloat* APIENTRY glcGetCharMetric(GLint inCode, GLCenum inMetric,
				   GLfloat *outVec)
{
  __glcContextState *state = NULL;
  GLint code = 0;
  GLfloat vector[14];

  assert(outVec);

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

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(state, inCode);
  if (code < 0)
    return NULL;

  /* Call __glcProcessChar that will get a font which maps the code to a glyph
   * or issue the replacement code or the character sequence \<xxx> and call
   * __glcGetCharMetric()
   */
  if (__glcProcessChar(state, code, 0, __glcGetCharMetric, vector)) {
    switch(inMetric) {
    case GLC_BASELINE:
      memcpy(outVec, vector, 4 * sizeof(GLfloat));
      return outVec;
    case GLC_BOUNDS:
      memcpy(outVec, &vector[4], 8 * sizeof(GLfloat));
      return outVec;
    }
  }

  return NULL;
}



/** \ingroup measure
 *  This command measures the layout that would result from rendering all
 *  mapped characters at the same origin. This contrast with
 *  glcGetStringCharMetric(), which measures characters as part of a string,
 *  that is, influenced by kerning, ligatures, and so on.
 *
 *  This command evaluates the metrics of every fonts in the
 *  \b GLC_CURRENT_FONT_LIST. Fonts that are not listed in
 *  \b GLC_CURRENT_FONT_LIST are ignored.
 *
 *  The command stores in \e outVec the value of the metric identified by
 *  \e inMetric. If the command does not raise an error, its return value
 *  is \e outVec.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e outVec is NULL.
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
GLfloat* APIENTRY glcGetMaxCharMetric(GLCenum inMetric, GLfloat *outVec)
{
  __glcContextState *state = NULL;
  GLfloat advance_x = 0., advance_y = 0., yb = 0., yt = 0., xr = 0., xl = 0.;
  FT_ListNode node = NULL;

  assert(outVec);

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

  /* For each font in GLC_CURRENT_FONT_LIST find the maximum values of the
   * advance width of the bounding boxes.
   */
  for (node = state->currentFontList.head; node; node = node->next) {
    GLfloat temp[6];
    __glcFont* font = (__glcFont*)node->data;

    if (!__glcFaceDescGetMaxMetric(font->faceDesc, temp, state))
      return NULL;

    advance_x = temp[0] > advance_x ? temp[0] : advance_x;
    advance_y = temp[1] > advance_y ? temp[1] : advance_y;
    yt = temp[2] > yt ? temp[2] : yt;
    yb = temp[3] < yb ? temp[3] : yb;
    xr = temp[4] > xr ? temp[4] : xr;
    xl = temp[5] < xl ? temp[5] : xl;
  }

  /* Update and transform, if necessary, the returned value */
  switch(inMetric) {
  case GLC_BASELINE:
    outVec[0] = 0.;
    outVec[1] = 0.;
    outVec[2] = advance_x;
    outVec[3] = advance_y;
    if (state->renderState.renderStyle == GLC_BITMAP)
      __glcTransformVector(&outVec[2], state->bitmapMatrix);
    return outVec;
  case GLC_BOUNDS:
    outVec[0] = xl;
    outVec[1] = yb;
    outVec[2] = xr;
    outVec[3] = yb;
    outVec[4] = xr;
    outVec[5] = yt;
    outVec[6] = xl;
    outVec[7] = yt;
    if (state->renderState.renderStyle == GLC_BITMAP) {
      int i = 0;

      for (i = 0; i < 4; i++)
	__glcTransformVector(&outVec[2*i], state->bitmapMatrix);
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
 *  \b GLC_MEASURED_CHAR_COUNT or \e outVec is NULL. If the command does not
 *  raise an error, its return value is outVec.
 *  \par Example:
 *  The following example first calls glcMeasureString() to store the string
 *  "hello" in the measurement buffer. It then retrieves both the baseline and
 *  the bounding box for the whole string, then for each individual character.
 *
 *  \code
 *  GLfloat overallBaseline[4];
 *  GLfloat overallBoundingBox[8];
 *
 *  GLfloat charBaslines[5][4];
 *  GLfloat charBoundingBoxes[5][8];
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
GLfloat* APIENTRY glcGetStringCharMetric(GLint inIndex, GLCenum inMetric,
				GLfloat *outVec)
{
  __glcContextState *state = NULL;
  GLfloat (*measurementBuffer)[12] = NULL;

  assert(outVec);

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

  measurementBuffer = (GLfloat(*)[12])GLC_ARRAY_DATA(state->measurementBuffer);

  /* Verify that inIndex is in legal bounds */
  if ((inIndex < 0)
      || (inIndex >= GLC_ARRAY_LENGTH(state->measurementBuffer))) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  switch(inMetric) {
  case GLC_BASELINE:
    memcpy(outVec, &measurementBuffer[inIndex][0],
           4*sizeof(GLfloat));
    return outVec;
  case GLC_BOUNDS:
    memcpy(outVec, &measurementBuffer[inIndex][4],
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
 *  The command raises \b GLC_PARAMETER_ERROR if \e outVec is \b GLC_NONE
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
GLfloat* APIENTRY glcGetStringMetric(GLCenum inMetric, GLfloat *outVec)
{
  __glcContextState *state = NULL;

  assert(outVec);

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

  /* Copy the values requested by the client in outVec */
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



/* This function perform the actual work of measuring a string
 * It is called by both glcMeasureString() and glcMeasureCountedString()
 * The string inString is encoded in UCS4 and is stored in visual order.
 */
static GLint __glcMeasureCountedString(__glcContextState *inState,
				GLboolean inMeasureChars, GLint inCount,
				const FcChar32* inString)
{
  GLint i = 0;
  GLfloat metrics[14] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
			 0.};
  const FcChar32* ptr = NULL;
  const GLint storeRenderStyle = inState->renderState.renderStyle;
  GLfloat xMin = 0., xMax = 0.;
  GLfloat yMin = 0., yMax = 0.;
  GLfloat* outVec = inState->measurementStringBuffer;
  GLint prevCode = 0;

  if (inState->renderState.renderStyle == GLC_BITMAP) {
     /* In order to prevent __glcProcessCharMetric() to transform its results
      * with the GLC_MATRIX, state->renderStyle must not be GLC_BITMAP
      */
    inState->renderState.renderStyle = 0;
  }

  memset(outVec, 0, 12*sizeof(GLfloat));

  /* For each character of the string, the measurement are performed and
   * gathered in the context state
   */
  ptr = inString;
  for (i = 0; i < inCount; i++) {
    __glcProcessChar(inState, *ptr, prevCode, __glcGetCharMetric, metrics);
    prevCode = *(ptr++);

    /* If characters are to be measured then store the results */
    if (inMeasureChars)
      __glcArrayAppend(inState->measurementBuffer, metrics);

    /* Initialize outVec if we are processing the first character of the string
     */
    if (!i) {
      outVec[0] = metrics[0];
      outVec[1] = metrics[1];
      outVec[2] = metrics[0];
      outVec[3] = metrics[1];
      outVec[4] = metrics[4] + metrics[0];
      outVec[5] = metrics[5] + metrics[1];
      outVec[6] = metrics[6] + metrics[0];
      outVec[9] = metrics[9] + metrics[1];
    }
    else {
      /* Takes the kerning into account */
      outVec[2] += metrics[12];
      outVec[3] += metrics[13];
      outVec[0] = outVec[2];
      outVec[1] = outVec[3];
    }

    xMin = metrics[4] + outVec[2];
    xMax = metrics[6] + outVec[2];
    yMin = metrics[5] + outVec[3];
    yMax = metrics[9] + outVec[3];

    outVec[4] = xMin < outVec[4] ? xMin : outVec[4];
    outVec[5] = yMin < outVec[5] ? yMin : outVec[5];
    outVec[6] = xMax > outVec[6] ? xMax : outVec[6];
    outVec[9] = yMax > outVec[9] ? yMax : outVec[9];

    outVec[2] += metrics[2];
    outVec[3] += metrics[3];
  }

  outVec[7] = outVec[5];
  outVec[8] = outVec[6];
  outVec[10] = outVec[4];
  outVec[11] = outVec[9];

  /* Transform all the data in the screen coordinate system if the rendering
   * style is GLC_BITMAP.
   */
  if (storeRenderStyle == GLC_BITMAP) {
    inState->renderState.renderStyle = storeRenderStyle;
    for (i = 0; i < 6; i++)
      __glcTransformVector(&inState->measurementStringBuffer[2*i],
			   inState->bitmapMatrix);
    if (inMeasureChars) {
      GLfloat (*measurementBuffer)[12] =
	(GLfloat(*)[12])GLC_ARRAY_DATA(inState->measurementBuffer);
      int j = 0;

      for (i = 0; i < inCount; i++) {
	for (j = 0; j < 6; j++)
	  __glcTransformVector(&measurementBuffer[i][2*j],
			       inState->bitmapMatrix);
      }
    }
  }

  /* Return the number of measured characters */
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
GLint APIENTRY glcMeasureCountedString(GLboolean inMeasureChars, GLint inCount,
			      const GLCchar* inString)
{
  __glcContextState *state = NULL;
  GLint count = 0;
  FcChar32* UinString = NULL;

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

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return 0;

  UinString = __glcConvertCountedStringToVisualUcs4(state, inString, inCount);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  count = __glcMeasureCountedString(state, inMeasureChars, inCount, UinString);

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
GLint APIENTRY glcMeasureString(GLboolean inMeasureChars, const GLCchar* inString)
{
  __glcContextState *state = NULL;
  FcChar32* UinString = NULL;
  GLint count = 0;
  GLint len = 0;
  FcChar32* ucs4 = NULL;

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return 0;

  UinString = __glcConvertToVisualUcs4(state, inString);
  if (!UinString) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  /* Compute the number of characters of the string so that there will be no
   * difference in the measurement process afterwards between the counted
   * strings and the other strings.
   */
  for (ucs4 = UinString, len = 0; *ucs4; ucs4++, len++);


  count = __glcMeasureCountedString(state, inMeasureChars, len, UinString);

  return count;
}
