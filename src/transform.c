/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
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
#include <string.h>
#include <math.h>

#include "GL/glc.h"
#include "internal.h"

#define GLC_PI 3.1415926535

/* glcLoadIdentity:
 *   This command assigns the value [1 0 0 1] to the floating point vector
 *   variable GLC_BITMAP_MATRIX
 */
void glcLoadIdentity(void)
{
    __glcContextState *state = NULL;

    /* Check if the current thread owns a context state */
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->bitmapMatrix[0] = 1.;
    state->bitmapMatrix[1] = 0.;
    state->bitmapMatrix[2] = 0.;
    state->bitmapMatrix[3] = 1.;
}

/* glcLoadMatrix:
 *   This command assigns the value [inMatrix[0] inMatrix[1] inMatrix[2]
 *   inMatrix[3]] to the floating point vector variable GLC_BITMAP_MATRIX
 */
void glcLoadMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;

    /* Check if the current thread owns a context state */
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(state->bitmapMatrix, inMatrix, 4 * sizeof(GLfloat));
}

/* glcMultMatrix:
 *   This command multiply the floating point vector variable GLC_BITMAP_MATRIX
 *   by in the incoming matrix inMatrix.
 */
void glcMultMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;
    GLfloat tempMatrix[4];

    /* Check if the current thread owns a context state */
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(tempMatrix, state->bitmapMatrix, 4 * sizeof(GLfloat));
    
    state->bitmapMatrix[0] = tempMatrix[0] * inMatrix[0] + tempMatrix[2] * inMatrix[1];
    state->bitmapMatrix[1] = tempMatrix[1] * inMatrix[0] + tempMatrix[3] * inMatrix[1];
    state->bitmapMatrix[2] = tempMatrix[0] * inMatrix[2] + tempMatrix[2] * inMatrix[3];
    state->bitmapMatrix[3] = tempMatrix[1] * inMatrix[2] + tempMatrix[3] * inMatrix[3];
}

/* glcRotate:
 *   This command assigns the value [a b c d] to the floating point vector
 *   variable GLC_BITMAP_MATRIX, where inAngle is measured in degrees,
 *   theta = inAngle * pi / 180 and
 *   [a c] = [matrix[0] matrix[2]] * [  cos(theta) sin(theta) ]
 *   [b d]   [matrix[1] matrix[3]]   [ -sin(theta) cos(theta) ]
 */
void glcRotate(GLfloat inAngle)
{
    GLfloat tempMatrix[4];
    GLfloat radian = inAngle * GLC_PI / 180.;
    GLfloat sine = sin(radian);
    GLfloat cosine = cos(radian);

    tempMatrix[0] = cosine;
    tempMatrix[1] = -sine;
    tempMatrix[2] = sine;
    tempMatrix[3] = cosine;
    
    glcMultMatrix(tempMatrix);
}

/* glcScale:
 *   This command assigns the value [a b c d] to the floating point vector
 *   variable GLC_BITMAP_MATRIX, where
 *   [a c] = [matrix[0] matrix[2]] * [ inX  0  ]
 *   [b d]   [matrix[1] matrix[3]]   [  0  inY ]
 */
void glcScale(GLfloat inX, GLfloat inY)
{
    GLfloat tempMatrix[4];

    tempMatrix[0] = inX;
    tempMatrix[1] = 0.;
    tempMatrix[2] = 0.;
    tempMatrix[3] = inY;
    
    glcMultMatrix(tempMatrix);
}
