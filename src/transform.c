/* $Id$ */
#include <string.h>
#include <math.h>

#include "GL/glc.h"
#include "internal.h"

void glcLoadIdentity(void)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    state->bitmapMatrix[0] = 1.;
    state->bitmapMatrix[1] = 0.;
    state->bitmapMatrix[2] = 0.;
    state->bitmapMatrix[3] = 1.;
}

void glcLoadMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(state->bitmapMatrix, inMatrix, 4 * sizeof(GLfloat));
}

void glcMultMatrix(const GLfloat *inMatrix)
{
    __glcContextState *state = NULL;
    GLfloat tempMatrix[4];

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    memcpy(tempMatrix, state->bitmapMatrix, 4 * sizeof(GLfloat));
    
    state->bitmapMatrix[0] = tempMatrix[0] * inMatrix[0] + tempMatrix[2] * inMatrix[1];
    state->bitmapMatrix[1] = tempMatrix[1] * inMatrix[0] + tempMatrix[3] * inMatrix[1];
    state->bitmapMatrix[2] = tempMatrix[0] * inMatrix[2] + tempMatrix[2] * inMatrix[3];
    state->bitmapMatrix[3] = tempMatrix[1] * inMatrix[2] + tempMatrix[3] * inMatrix[3];
}

void glcRotate(GLfloat inAngle)
{
    GLfloat tempMatrix[4];
    GLfloat radian = inAngle * M_PI / 180.;
    GLfloat sine = sin(radian);
    GLfloat cosine = cos(radian);

    tempMatrix[0] = cosine;
    tempMatrix[1] = sine;
    tempMatrix[2] = -sine;
    tempMatrix[3] = cosine;
    
    glcMultMatrix(tempMatrix);
}

void glcScale(GLfloat inX, GLfloat inY)
{
    GLfloat tempMatrix[4];

    tempMatrix[0] = inX;
    tempMatrix[1] = 1.;
    tempMatrix[2] = 1.;
    tempMatrix[3] = inY;
    
    glcMultMatrix(tempMatrix);
}
