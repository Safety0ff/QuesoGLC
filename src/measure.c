/* $Id$ */
#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_GLYPH_H

static void __glcTransformVector(GLfloat* outVec, __glcContextState *inState)
{
    GLfloat* matrix = inState->bitmapMatrix;
    GLfloat temp = 0.;
    
    temp = matrix[0] * outVec[0] + matrix[2] * outVec[1];
    outVec[1] = matrix[1] * outVec[0] + matrix[3] * outVec[1];
    outVec[0] = temp;
}

static GLfloat* __glcGetCharMetric(GLint inCode, GLCenum inMetric, GLfloat *outVec, GLint inFont, __glcContextState *inState)
{
    __glcFont *font = inState->fontList[inFont - 1];
    FT_Face face = font->face;
    FT_Glyph_Metrics metrics = face->glyph->metrics;
    FT_Glyph glyph = NULL;
    FT_BBox boundBox;
    GLint i = 0;
    
    /* TODO : use a dichotomic algo. instead*/
    for (i = 0; i < font->charMapCount; i++) {
	if (inCode == font->charMap[0][i]) {
	    inCode = font->charMap[1][i];
	    break;
	}
    }
    
    if (FT_Load_Char(face, inCode, FT_LOAD_NO_SCALE)) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
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
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return NULL;
    }

    font = state->getFont(inCode);
    if (font)
	return __glcGetCharMetric(inCode, inMetric, outVec, font, state);

    repCode = glcGeti(GLC_REPLACEMENT_CODE);
    font = state->getFont(repCode);
    if (repCode && font)
	return __glcGetCharMetric(repCode, inMetric, outVec, font, state);

    /* Here GLC must render /<hexcode>. Should we return the metrics of
       the whole string or nothing at all ? */
    return NULL;
}

GLfloat* glcGetMaxCharMetric(GLCenum inMetric, GLfloat *outVec)
{
    __glcContextState *state = NULL;
    FT_Face face = NULL;
    GLfloat advance = 0., yb = 0., yt = 0., xr = 0., xl = 0.;
    GLint i = 0;

    switch(inMetric) {
	case GLC_BASELINE:
	case GLC_BOUNDS:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return NULL;
    }

    for (i = 0; i < state->currentFontCount; i++) {
	GLfloat temp = 0.;
	face = state->fontList[state->currentFontList[i]]->face;
	
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

GLfloat* glcGetStringCharMetric(GLint inIndex, GLCenum inMetric, GLfloat *outVec)
{
    __glcContextState *state = NULL;
    
    switch(inMetric) {
	case GLC_BASELINE:
	case GLC_BOUNDS:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return NULL;
    }

    if ((inIndex < 0) || (inIndex >= state->measuredCharCount)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
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
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return NULL;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
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
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
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
	  horizontally from left to right.
     */
    for (i = 0; i < inCount; i++) {
	/* For each character get the metrics */
	glcGetCharMetric(s[i], GLC_BASELINE, baselineMetric);
	glcGetCharMetric(s[i], GLC_BOUNDS, boundsMetric);
	
	/* If character are to be measured then store the results */
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

GLint glcMeasureString(GLboolean inMeasureChars, const GLCchar* inString)
{
    char *s = (char *)inString;
    
    return glcMeasureCountedString(inMeasureChars, strlen(s), inString);
}
