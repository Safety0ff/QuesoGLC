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
 * defines the so-called "Transformation commands" described in chapter 3.9
 * of the GLC specs.
 */

/** \defgroup transform Transformation commands
 *  The GLC transformation commands modify the value of \b GLC_BITMAP_MATRIX.
 *  Glyph coordinates are defined in the em coordinate system. When the value
 *  of \b GLC_RENDER_STYLE is \b GLC_BITMAP, GLC uses the 2x2
 *  \b GLC_BITMAP_MATRIX to transform layouts from the em coordinate system to
 *  the GL raster coordinate system in which bitmaps are drawn.
 *
 *  When the value of the variable \b GLC_RENDER_STYLE is not \b GLC_BITMAP,
 *  GLC performs no transformations on glyph coordinates. In this case, GLC
 *  uses em coordinates directly as GL world coordinates when drawing a layout,
 *  and it is the responsibility of the GLC client to issue GL commands that
 *  set up the appropriate GL transformations.
 */

#include <math.h>

#include "GL/glc.h"
#include "internal.h"

#define GLC_PI 3.1415926535



/** \ingroup transform
 *  This command assigns the value [1 0 0 1] to the floating point vector
 *  variable \b GLC_BITMAP_MATRIX.
 *  \sa glcGetfv() with argument GLC_BITMAP_MATRIX
 *  \sa glcLoadMatrix()
 *  \sa glcMultMatrix()
 *  \sa glcRotate()
 *  \sa glcScale()
 */
void APIENTRY glcLoadIdentity(void)
{
    __glcContextState *state = NULL;

    /* Check if the current thread owns a context state */
    state = __glcGetCurrent();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    state->bitmapMatrix[0] = 1.;
    state->bitmapMatrix[1] = 0.;
    state->bitmapMatrix[2] = 0.;
    state->bitmapMatrix[3] = 1.;
}



/** \ingroup transform
 *  This command assigns the value [inMatrix[0] inMatrix[1] inMatrix[2]
 *  inMatrix[3]] to the floating point vector variable \b GLC_BITMAP_MATRIX.
 *
 *  \param inMatrix The value to assign to \b GLC_BITMAP_MATRIX
 *  \sa glcGetfv() with argument GLC_BITMAP_MATRIX
 *  \sa glcLoadIdentity()
 *  \sa glcMultMatrix()
 *  \sa glcRotate()
 *  \sa glcScale()
 */
void APIENTRY glcLoadMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;

    assert(inMatrix);

    /* Check if the current thread owns a context state */
    state = __glcGetCurrent();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(state->bitmapMatrix, inMatrix, 4 * sizeof(GLfloat));
}



/** \ingroup transform
 *  This command multiply the floating point vector variable
 *  \b GLC_BITMAP_MATRIX by the incoming matrix \e inMatrix.
 *
 *  \param inMatrix A pointer to a 2x2 matrix stored in column-major order
 *                  as 4 consecutives values.
 *  \sa glcGetfv() with argument GLC_BITMAP_MATRIX
 *  \sa glcLoadIdentity()
 *  \sa glcLoadMatrix()
 *  \sa glcRotate()
 *  \sa glcScale()
 */
void APIENTRY glcMultMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;
    GLfloat tempMatrix[4];

    assert(inMatrix);

    /* Check if the current thread owns a context state */
    state = __glcGetCurrent();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(tempMatrix, state->bitmapMatrix, 4 * sizeof(GLfloat));

    state->bitmapMatrix[0] = tempMatrix[0] * inMatrix[0]
      + tempMatrix[2] * inMatrix[1];
    state->bitmapMatrix[1] = tempMatrix[1] * inMatrix[0]
      + tempMatrix[3] * inMatrix[1];
    state->bitmapMatrix[2] = tempMatrix[0] * inMatrix[2]
      + tempMatrix[2] * inMatrix[3];
    state->bitmapMatrix[3] = tempMatrix[1] * inMatrix[2]
      + tempMatrix[3] * inMatrix[3];
}



/** \ingroup transform
 *  This command assigns the value [a b c d] to the floating point vector
 *  variable \b GLC_BITMAP_MATRIX, where \e inAngle is measured in degrees,
 *  <em>theta = inAngle * pi / 180</em> and \n
 *  <pre>[a c] = [matrix[0] matrix[2]] * [  cos(theta) sin(theta) ]</pre>
 *  <pre>[b d]   [matrix[1] matrix[3]]   [ -sin(theta) cos(theta) ]</pre>
 *  \param inAngle The angle of rotation around the Z axis, in degrees.
 *  \sa glcGetfv() with argument GLC_BITMAP_MATRIX
 *  \sa glcLoadIdentity()
 *  \sa glcLoadMatrix()
 *  \sa glcMultMatrix()
 *  \sa glcScale()
 */
void APIENTRY glcRotate(GLfloat inAngle)
{
    GLfloat tempMatrix[4];
    GLfloat radian = inAngle * GLC_PI / 180.;
    GLfloat sine = sin(radian);
    GLfloat cosine = cos(radian);

    tempMatrix[0] = cosine;
    tempMatrix[1] = sine;
    tempMatrix[2] = -sine;
    tempMatrix[3] = cosine;

    glcMultMatrix(tempMatrix);
}



/** \ingroup transform
 *  This command produces a general scaling along the \b x and \b y
 *  axes, that is, it assigns the value [a b c d] to the floating point
 *  vector variable GLC_BITMAP_MATRIX, where
 *  <pre>[a c] = [matrix[0] matrix[2]] * [ inX  0  ]</pre>
 *  <pre>[b d]   [matrix[1] matrix[3]]   [  0  inY ]</pre>
 *  \param inX The scale factor along the \b x axis
 *  \param inY The scale factor along the \b y axis
 *  \sa glcGetfv() with argument GLC_BITMAP_MATRIX
 *  \sa glcLoadIdentity()
 *  \sa glcLoadMatrix()
 *  \sa glcMultMatrix()
 *  \sa glcRotate()
 */
void APIENTRY glcScale(GLfloat inX, GLfloat inY)
{
    GLfloat tempMatrix[4];

    tempMatrix[0] = inX;
    tempMatrix[1] = 0.;
    tempMatrix[2] = 0.;
    tempMatrix[3] = inY;

    glcMultMatrix(tempMatrix);
}
