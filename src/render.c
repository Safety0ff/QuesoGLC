/* $Id$ */
#include <string.h>
#include <stdio.h>

#include "GL/glc.h"
#include "internal.h"

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

GLint __glcGetFont(GLint inCode)
{
    GLCfunc callbackFunc = NULL;
    GLint font = 0;
    
    font = __glcLookupFont(inCode);
    if (font)
	return font;

    callbackFunc = glcGetCallbackFunc(GLC_OP_glcUnmappedCode);
    if (callbackFunc && (*callbackFunc)(inCode)) {
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

static void __glcRenderCharBitmap(GLint inGlyphIndex, __glcFont* inFont, __glcContextState* inState)
{
}

static void __glcRenderCharTexture(GLint inGlyphIndex, __glcFont* inFont, __glcContextState* inState)
{
}

static void __glcRenderChar(GLint inCode, GLint inFont)
{
    __glcContextState *state = __glcGetCurrentState();
    __glcFont* font = state->fontList[inFont - 1];
    GLint glyphIndex = 0;
    
    if (FT_Set_Char_Size(font->face, 0, 1 << 16, 0, 0)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    
    switch(state->renderStyle) {
	case GLC_BITMAP:
	    __glcRenderCharBitmap(glyphIndex, font, state);
	    return;
	case GLC_TEXTURE:
	    __glcRenderCharTexture(glyphIndex, font, state);
	    return;
	case GLC_LINE:
	case GLC_TRIANGLE:
	    if (FT_Load_Glyph(font->face, glyphIndex, FT_LOAD_DEFAULT)) {
		__glcRaiseError(GLC_INTERNAL_ERROR);
		return;
	    }
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

    /* Save current GL state */
    for (i = 0; i < inCount; i++) {
	glcRenderChar(s[i]);
	/* Advance pen for next character */
    }
    /* Restore current GL state */
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
