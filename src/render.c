/* $Id$ */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_GLYPH_H
#include FT_OUTLINE_H

#define GLC_TEXTURE_SIZE 64

static GLint __glcLookupFont(GLint inCode)
{
    GLint i = 0;
    const GLint n = glcGeti(GLC_CURRENT_FONT_COUNT);
    
    for (i = 0; i < n; i++) {
	const GLint font = glcGetListi(GLC_CURRENT_FONT_LIST, i);
	if (glcGetFontMap(font, inCode))
	    return font;
    }
    return 0;
}

static GLboolean __glcCallCallbackFunc(GLint inCode)
{
    GLCfunc callbackFunc = NULL;
    GLboolean result = GL_FALSE;
    __glcContextState *state = NULL;
    
    callbackFunc = glcGetCallbackFunc(GLC_OP_glcUnmappedCode);
    if (!callbackFunc)
	return GL_FALSE;

    state = __glcGetCurrentState();
    state->isInCallbackFunc = GL_TRUE;
    result = (*callbackFunc)(inCode);
    state->isInCallbackFunc = GL_FALSE;
    
    return result;
}

GLint __glcGetFont(GLint inCode)
{
    GLint font = 0;
    
    font = __glcLookupFont(inCode);
    if (font)
	return font;

    if (__glcCallCallbackFunc(inCode)) {
	font = __glcLookupFont(inCode);
	if (font)
	    return font;
    }

    if (glcIsEnabled(GLC_AUTO_FONT)) {
	GLint i = 0;
	GLint n = 0;
	
	n = glcGeti(GLC_FONT_COUNT);
	for (i = 0; i < n; i++) {
	    font = glcGetListi(GLC_FONT_LIST, i);
	    if (glcGetFontMap(font, inCode)) {
		glcAppendFont(font);
		return font;
	    }
	}
	
	n = glcGeti(GLC_MASTER_COUNT);
	for (i = 0; i < n; i++) {
	    if (glcGetMasterMap(i, inCode)) {
		font = glcNewFontFromMaster(glcGenFontID(), i);
		if (font) {
		    glcAppendFont(font);
		    return font;
		}
	    }
	}
    }
    return 0;
}

/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(__glcFont* inFont, __glcContextState* inState)
{
    FT_Matrix matrix;
    FT_Face face = inFont->face;
    FT_Outline outline = face->glyph->outline;
    FT_BBox boundBox;
    FT_Bitmap pixmap;
    GLint EM_size = face->units_per_EM;

    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed) (inState->bitmapMatrix[0] * 65536. / EM_size);
    matrix.xy = (FT_Fixed) (inState->bitmapMatrix[2] * 65536. / EM_size);
    matrix.yx = (FT_Fixed) (inState->bitmapMatrix[1] * 65536. / EM_size);
    matrix.yy = (FT_Fixed) (inState->bitmapMatrix[3] * 65536. / EM_size);
    
    FT_Outline_Transform(&outline, &matrix);
    FT_Outline_Get_CBox(&outline, &boundBox);
    
    boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
    boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
    boundBox.xMax = (boundBox.xMax + 32) & -64;	/* ceiling(xMax) */
    boundBox.yMax = (boundBox.yMax + 32) & -64;	/* ceiling(yMax) */
    
    pixmap.width = (boundBox.xMax - boundBox.xMin) / 64;
    pixmap.rows = (boundBox.yMax - boundBox.yMin) / 64;
    pixmap.pitch = pixmap.width / 8 + 1;	/* 1 bit / pixel */
    
    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
    pixmap.buffer = (GLubyte *)malloc(pixmap.rows * pixmap.pitch);
    if (!pixmap.buffer) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);
    
    /* Flip the picture */
    pixmap.pitch = - pixmap.pitch;
    
    /* translate the outline to match (0, 0) with the glyph's lower left corner */
    FT_Outline_Translate(&outline, -boundBox.xMin, -boundBox.yMin);
    
    /* render the glyph */
    if (FT_Outline_Get_Bitmap(library, &outline, &pixmap)) {
	free(pixmap.buffer);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(pixmap.width, pixmap.rows, -boundBox.xMin / 64., -boundBox.yMin / 64.,
	face->glyph->advance.x * inState->bitmapMatrix[0] / EM_size / 64., 
	face->glyph->advance.x * inState->bitmapMatrix[1] / EM_size / 64.,
	pixmap.buffer);
    
    free(pixmap.buffer);
}

static void __glcRenderCharTexture(__glcFont* inFont, __glcContextState* inState)
{
    FT_Matrix matrix;
    FT_Face face = inFont->face;
    FT_Outline outline = face->glyph->outline;
    FT_BBox boundBox;
    FT_Bitmap pixmap;
    GLint EM_size = face->units_per_EM;
    GLuint texture = 0;
    GLfloat width = 0, height = 0;
    GLint format = 0;

    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_LUMINANCE, GLC_TEXTURE_SIZE,
	GLC_TEXTURE_SIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &format);
    if (!format) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed) (GLC_TEXTURE_SIZE * 65536. / EM_size);
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = (FT_Fixed) (GLC_TEXTURE_SIZE * 65536. / EM_size);
    
    FT_Outline_Transform(&outline, &matrix);
    FT_Outline_Get_CBox(&outline, &boundBox);
    
    boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
    boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
    boundBox.xMax = (boundBox.xMax + 32) & -64;	/* ceiling(xMax) */
    boundBox.yMax = (boundBox.yMax + 32) & -64;	/* ceiling(yMax) */
    
    width = (GLfloat)((boundBox.xMax - boundBox.xMin) / 64);
    height = (GLfloat)((boundBox.yMax - boundBox.yMin) / 64);
    pixmap.width = GLC_TEXTURE_SIZE;
    pixmap.rows = GLC_TEXTURE_SIZE;
    pixmap.pitch = pixmap.width;		/* 8 bits / pixel */
    
    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_grays;	/* Anti-aliased rendering */
    pixmap.num_grays = 256;
    pixmap.buffer = (GLubyte *)malloc(pixmap.rows * pixmap.pitch);
    if (!pixmap.buffer) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);

    /* Flip the picture */
    pixmap.pitch = - pixmap.pitch;
    
    /* translate the outline to match (0, 0) with the glyph's lower left corner */
    FT_Outline_Translate(&outline, -boundBox.xMin, -boundBox.yMin);
    
    /* render the glyph */
    if (FT_Outline_Get_Bitmap(library, &outline, &pixmap)) {
	free(pixmap.buffer);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }

    if ((inState->glObjects)
    && (inState->textureObjectCount <= GLC_MAX_TEXTURE_OBJECT)) {
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	inState->textureObjectList[inState->textureObjectCount] = texture;
	inState->textureObjectCount++;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (inState->mipmap) {
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE8, GLC_TEXTURE_SIZE,
		GLC_TEXTURE_SIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		pixmap.buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, GLC_TEXTURE_SIZE,
		GLC_TEXTURE_SIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		pixmap.buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex2f(boundBox.xMin / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMin / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(width / GLC_TEXTURE_SIZE, 0.);
	glVertex2f(boundBox.xMax / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMin / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(width / GLC_TEXTURE_SIZE, height / GLC_TEXTURE_SIZE);
	glVertex2f(boundBox.xMax / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMax / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(0., height / GLC_TEXTURE_SIZE);
	glVertex2f(boundBox.xMin / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMax / 64. / GLC_TEXTURE_SIZE);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glTranslatef(face->glyph->advance.x * inState->bitmapMatrix[0] / EM_size / 64., 
	face->glyph->advance.x * inState->bitmapMatrix[1] / EM_size / 64., 0.);
    
    free(pixmap.buffer);
}

static void __glcRenderChar(GLint inCode, GLint inFont)
{
    __glcContextState *state = __glcGetCurrentState();
    __glcFont* font = state->fontList[inFont - 1];
    GLint glyphIndex = 0;
    GLint i = 0;
    
    /* TODO : use a dichotomic algo. instead*/
    for (i = 0; i < font->charMapCount; i++) {
	if (inCode == font->charMap[0][i]) {
	    inCode = font->charMap[1][i];
	    break;
	}
    }
    
    if (FT_Set_Char_Size(font->face, 0, 1 << 16, state->resolution,  state->resolution)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    
    if (FT_Load_Glyph(font->face, glyphIndex, FT_LOAD_DEFAULT)) {
	__glcRaiseError(GLC_INTERNAL_ERROR);
	return;
    }

    switch(state->renderStyle) {
	case GLC_BITMAP:
	    __glcRenderCharBitmap(font, state);
	    return;
	case GLC_TEXTURE:
	    __glcRenderCharTexture(font, state);
	    return;
	case GLC_LINE:
	case GLC_TRIANGLE:
	    __glcRenderCharScalable(font, state, (state->renderStyle == GLC_TRIANGLE));
	    return;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return;
    }
}

void glcRenderChar(GLint inCode)
{
    GLint repCode = 0;
    GLint font = 0;
    
    if (!glcGetCurrentContext()) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    font = __glcGetFont(inCode);
    if (font) {
	__glcRenderChar(inCode, font);
	return;
    }

    repCode = glcGeti(GLC_REPLACEMENT_CODE);
    font = __glcGetFont(repCode);
    if (repCode && font) {
	__glcRenderChar(repCode, font);
	return;
    }
    else {
	/* FIXME : use Unicode instead of ASCII */
	char buf[10];
	GLint i = 0;
	GLint n = 0;
	
	if (!__glcGetFont('\\') || !__glcGetFont('<') || !__glcGetFont('>'))
	    return;

	sprintf(buf,"%X", inCode);
	n = strlen(buf);
	for (i = 0; i < n; i++) {
	    if (!__glcGetFont(buf[i]))
		return;
	}
	
	__glcRenderChar('\\', __glcGetFont('\\'));
	__glcRenderChar('<', __glcGetFont('<'));
	for (i = 0; i < n; i++)
	    __glcRenderChar(buf[i], __glcGetFont(buf[i]));
	__glcRenderChar('>', __glcGetFont('>'));
    }
}

void glcRenderCountedString(GLint inCount, const GLCchar *inString)
{
    GLint i = 0;
    char *s = (char *)inString;
    
    if (!glcGetCurrentContext()) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    /* FIXME : use Unicode instead of ASCII */

    for (i = 0; i < inCount; i++)
	glcRenderChar(s[i]);
}

void glcRenderString(const GLCchar *inString)
{
    glcRenderCountedString(strlen((char *)inString), inString);
}

void glcRenderStyle(GLCenum inStyle)
{
    __glcContextState *state = NULL;

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
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    state->renderStyle = inStyle;
    return;
}

void glcReplacementCode(GLint inCode)
{
    __glcContextState *state = NULL;

    switch(inCode) {
	case GLC_REPLACEMENT_CODE:
	    break;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return;
    }
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    state->replacementCode = inCode;
    return;
}

void glcResolution(GLfloat inVal)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    state->resolution = inVal;
    return;
}
