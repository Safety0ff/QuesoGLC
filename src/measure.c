/* $Id$ */
#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"

static void __glcTransformVector(GLfloat* outVec, __glcContextState *inState)
{
    GLfloat* matrix = inState->bitmapMatrix;
    GLfloat temp = 0.;
    
    temp = matrix[0] * outVec[0] + matrix[2] * outVec[1];
    outVec[1] = matrix[1] * outVec[0] + matrix[3] * outVec[1];
    outVec[0] = temp;
}

static GLfloat* __glcGetCharMetric(GLint inCode, GLCenum, GLfloat *outVec, GLint inFont, __glcContextState *inState)
{
    FT_Face face = inState->fontList[inFont - 1]->face;
    FT_Glyph_Metrics metrics = face->glyph->metrics;
    
    if (FT_Load_Char(face, inCode, FT_LOAD_LINEAR_DESIGN)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
    }
    
    switch(inMetric) {
	case GLC_BASELINE:
	    outVec[0] = 0.;
	    outVec[1] = 0.;
	    outVec[2] = metrics->horiAdvance;
	    outVec[3] = metrics->vertAdvance;
	    if (inState->renderStyle == GLC_BITMAP)
		__glcTransformVector(&outVec[2]);
	    return outVec;
	case GLC_BOUNDS:
	    outVec[0] = metrics->horiBearingX;
	    outVec[1] = metrics->vertBearingY - metrics->height;
	    outVec[2] = outVec[0] + metrics->width;
	    outVec[3] = outVec[1];
	    outVec[4] = outVec[2];
	    outVec[5] = metrics->vertBearingY;
	    outVec[6] = outVec[0];
	    outVec[7] = outVec[5];
	    if (inState->renderStyle == GLC_BITMAP) {
		__glcTransformVector(&outVec[0]);
		__glcTransformVector(&outVec[2]);
		__glcTransformVector(&outVec[4]);
		__glcTransformVector(&outVec[6]);
	    }
	    return outVec;
    }
    
    return NULL;
}

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
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return NULL;
    }

    font = __glcGetFont(inCode);
    if (font)
	return __glcGetCharMetric(inCode, inMetric, outVec, font, state);

    repCode = glcGeti(GLC_REPLACEMENT_CODE);
    font = __glcGetFont(repCode);
    if (repCode && font)
	return __glcGetCharMetric(repCode, inMetric, outVec, font, state);

    /* Here GLC must render /<hexcode>. Should we return the metrics of
       the whole string or nothing at all ? */
    return NULL;
}

GLfloat* glcGetMaxCharMetric(GLCenum inMetric, GLfloat *outVec)
{
    __glcContextState *state = NULL;
    FT_Size faceSize = NULL;
    GLfloat xr = 0., yb = 0., yt = 0.

    switch(inMetric) {
	case GLC_BASELINE:
	case GLC_BOUNDS:
	    break;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return NULL;
    }

    for (i = 0; i < state->currentFontCount; i++) {
	GLfloat temp = 0.;
	faceSize = state->fontList[state->currentFontList[i]]->face->size;
	
	/* These are estimations since there is no way to compute the
	   max layouts without drawing ALL the glyphs (which should be
	   a big performance hit) !!!
	   Use face->bbox instead...
	 */
	temp = (GLfloat)faceSize->max_advance / x_ppem / 65536.;
	if (temp > xr)
	    xr =temp;
	
	temp = (GLfloat)faceSize->ascender / y_ppem / 65536.;
	if (temp > yt)
	    yt =temp;
	
	temp = (GLfloat)faceSize->descender / y_ppem / 65536.;
	if (temp > yb)
	    xr =temp;
    }
    
    outVec[0] = 0.;
    
    switch(inMetric) {
	case GLC_BASELINE:
	    outVec[1] = 0.;
	    outVec[2] = xr;
	    outVec[3] = 0.;
	    if (inState->renderStyle == GLC_BITMAP)
		__glcTransformVector(&outVec[2]);
	    return outVec;
	case GLC_BOUNDS:
	    outVec[1] = -yb;
	    outVec[2] = xr;
	    outVec[3] = outVec[1];
	    outVec[4] = outVec[2];
	    outVec[5] = yt;
	    outVec[6] = outVec[0];
	    outVec[7] = outVec[5];
	    if (inState->renderStyle == GLC_BITMAP) {
		__glcTransformVector(&outVec[0]);
		__glcTransformVector(&outVec[2]);
		__glcTransformVector(&outVec[4]);
		__glcTransformVector(&outVec[6]);
	    }
	    return outVec;
    }
    
    return NULL;
}

GLfloat* glcGetStringCharMetric(GLint inIndex, GLCenum inMetric, GLfloat *outVec)
{
    __glcContextState *state = NULL;
    
    switch(inMetric) {
	case GLC_BASELINE:
	case GLC_BOUNDS:
	    break;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return NULL;
    }

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
	    return outVec;
    }
    
    return NULL;
}

GLfloat* glcGetStringMetric(GLCenum inMetric, GLfloat *outVec)
{
    __glcContextState *state = NULL;
    
    switch(inMetric) {
	case GLC_BASELINE:
	case GLC_BOUNDS:
	    break;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcGetCurrentState();
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
	    return outVec;
    }
    
    return NULL;
}

GLint glcMeasureCountedString(GLboolean inMeasureChars, GLint inCount, const GLCchar* inString)
{
    __glcContextState *state = NULL;
    GLint i = 0, j = 0;
    GLfloat baselineMetric[4] = {0., 0., 0., 0.};
    GLfloat boundsMetric[8] = {0., 0., 0., 0., 0., 0., 0., 0.};
    GLfloat storeBitmapMatrix[4] = {0., 0., 0., 0.};
    char *s = (char *)inString;

    if (inCount <= 0) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    if (state->renderStyle == GLC_BITMAP) {
	for (i = 0; i < 4; i++)
	    storeBitmapMatrix[i] = state->bitmapMatrix[i];
	glcLoadIdentity();
    }

    for (i = 0; i < 12; i++)
	state->measurementStringBuffer[i] = 0.;
    
    /* FIXME :
       1. Use Unicode instead of ASCII
       2. Computations performed below assume that the string is rendered
	  horizontally
     */
    for (i = 0; i < inCount; i++) {
	glcGetCharMetric(s[i], GLC_BASELINE, baselineMetric);
	glcGetCharMetric(s[i], GLC_BOUNDS, boundsMetric);
	if (inMeasureChars) {
	    for (j = 0; j < 4; j++)
		state->measurementCharBuffer[i][j] = baselineMetric[j];
	    for (j = 0; j < 8; j++)
		state->measurementCharBuffer[i][j + 4] = boundsMetric[j];
	}
	if (!i) {
	    state->measurementStringBuffer[4] = boundsMetric[0];
	    state->measurementStringBuffer[10] = boundsMetric[0];
	}
	state->measurementStringBuffer[2] += baselineMetric[2];
	if (state->measurementStringBuffer[5] > boundsMetric[1]) {
	    state->measurementStringBuffer[5] = boundsMetric[1];
	    state->measurementStringBuffer[7] = boundsMetric[1];
	}
	if (state->measurementStringBuffer[9] < boundsMetric[5]) {
	    state->measurementStringBuffer[9] = boundsMetric[5];
	    state->measurementStringBuffer[11] = boundsMetric[5];
	}
	state->measurementStringBuffer[6] += boundsMetric[4];
	state->measurementStringBuffer[8] += boundsMetric[4];
    }
    
    if (state->renderStyle == GLC_BITMAP) {
	for (i = 0; i < 4; i++)
	    state->bitmapMatrix[i] = storeBitmapMatrix[i];
	__glcTransformVector(&state->measurementStringBuffer[2]);
	__glcTransformVector(&state->measurementStringBuffer[4]);
	__glcTransformVector(&state->measurementStringBuffer[6]);
	__glcTransformVector(&state->measurementStringBuffer[8]);
	__glcTransformVector(&state->measurementStringBuffer[10]);
	if (inMeasureChar)
	    for (i = 0; i < inCount; i++) {
		__glcTransformVector(&state->measurementCharBuffer[2]);
		__glcTransformVector(&state->measurementCharBuffer[4]);
		__glcTransformVector(&state->measurementCharBuffer[6]);
		__glcTransformVector(&state->measurementCharBuffer[8]);
		__glcTransformVector(&state->measurementCharBuffer[10]);
	    }
    }
    
    if (inMeasureChar)
	state->measuredCharCount = inCount;
    else
	state->measuredCharCount = 0;

    return inCount;
}

GLint glcMeasureString(GLboolean inMeasureChars, const GLCchar* inString)
{
    char *s = (char *)inString;
    
    return glcMeasureCountedString(inMeasureChars, strlen(s), inString);
}
