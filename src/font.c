/* $Id$ */
#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"

void glcAppendFont(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    if (!glcIsFont(inFont))
	return;
    
    if (state->currentFontCount < GLC_MAX_CURRENT_FONT) {
	state->currentFontList[state->currentFontCount] = inFont;
	state->currentFontCount++;
    }
    else
	__glcRaiseError(GLC_RESOURCE_ERROR);

    return;
}

static void __glcDeleteFont(GLint inFont, __glcContextState* state)
{
    int i = 0;
    
    for(i = 0; i < state->currentFontCount; i++) {
	if (state->currentFontList[i] == inFont)
	    break;
    }
    
    if (i < state->currentFontCount) {
	memmove(&state->currentFontList[i], &state->currentFontList[i+1], (GLC_MAX_CURRENT_FONT - i -1) * sizeof(GLint));
	state->currentFontCount--;
    }
}

void glcDeleteFont(GLint inFont)
{
    __glcContextState *state = NULL;
    __glcFont *font = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    if (!glcIsFont(inFont))
	return;

    __glcDeleteFont(inFont, state);
    font = state->fontList[inFont - 1];
    FT_Done_Face(font->face);
    free(font);
    state->fontList[inFont - 1] = NULL;
    state->fontCount--;
}

void glcFont(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    if (!glcIsFont(inFont))
	return;
    
    state->currentFontList[0] = inFont;
    state->currentFontCount = 1;
}

static GLboolean __glcFontFace(GLint inFont, const GLCchar* inFace, __glcContextState *inState)
{
    __glcFont *font = NULL;
    GLint faceID = 0;
    char buffer[256];

    font = inState->fontList[inFont];
    
    if (!font) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return GL_FALSE;
    }
    
    faceID = __glcStringListGetIndex(&font->parent->faceList, inFace);
    if (faceID == -1)
	return GL_FALSE;

    font->faceID = faceID;

    if (FT_New_Face(library, __glcStringListExtractElement(&font->parent->faceFileName,
		faceID, buffer, 256), 0, &font->face)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return GL_FALSE;
    }
    
    return GL_TRUE;
}

static __glcFont* __glcVerifyFontParameters(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return NULL;
    }

    if ((inFont < 1) || (inFont >= GLC_MAX_FONT)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return NULL;
    }

    return state->fontList[inFont - 1];
}

GLboolean glcFontFace(GLint inFont, const GLCchar* inFace)
{
    __glcContextState *state = NULL;
    __glcFont *font = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return GL_FALSE;
    }

    if (!glcIsFont(inFont))
	return GL_FALSE;

    font = state->fontList[inFont - 1];
    
    if (inFont) {
	FT_Done_Face(font->face);
	return __glcFontFace(inFont - 1, inFace, state);
    }
    else {
	int i = 0;
	GLboolean result = GL_TRUE;
	
	for(i = 0; i < state->currentFontCount; i++) {
	    FT_Done_Face(state->fontList[state->currentFontList[i] - 1]->face);
	    result |= __glcFontFace(state->currentFontList[i] - 1, inFace, state);
	}
	
	return result;
    }
}

void glcFontMap(GLint inFont, GLint inCode, const GLCchar* inCharName)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    FT_UInt glyphIndex = 0;
    GLint i = 0;
    GLint code  = 0;
    datum key, content;
    
    if (font) {
	/* Verify that the glyph exists in the face */
	glyphIndex = FT_Get_Char_Index(font->face, inCode);
	if (!glyphIndex) {
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return;
	}
	/* Retrieve the Unicode code from its name */
	key.dsize = strlen(inCharName) + 1;
	key.dptr = (char *)inCharName;
	content = gdbm_fetch(unicod2, key);
	if (!content.dptr) {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    return;
	}
	code = (GLint)(*(content.dptr));
	free(content.dptr);
	/* Use a dichotomic algo instead */
	for (i = 0; i < font->charMapCount; i++) {
	    if (font->charMap[0][i] >= code)
		break;
	}
	if (font->charMap[0][i] != code) {
	    /* If the char is not yet registered then add it */
	    if (font->charMapCount < GLC_MAX_CHARMAP) {
		memmove(&font->charMap[0][i+1], &font->charMap[0][i], 
			(font->charMapCount - i) * 2 * sizeof(GLint));
		font->charMapCount++;
		font->charMap[0][i] = code;
	    }
	    else {
		__glcRaiseError(GLC_RESOURCE_ERROR);
		return;
	    }
	}
	font->charMap[1][i] = inCode;
    }
    else {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return;
    }
}

GLint glcGenFontID(void)
{
    __glcContextState *state = NULL;
    int i = 0;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    for (i = 0; i < GLC_MAX_FONT; i++) {
	if (!state->fontList[i])
	    break;
    }
    
    if (i < GLC_MAX_FONT)
	return i + 1;
    else {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
}

const GLCchar* glcGetFontFace(GLint inFont)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	return __glcStringListExtractElement(&font->parent->faceList, font->faceID, (GLCchar *)__glcBuffer, GLC_STRING_CHUNK);
    else
	return GLC_NONE;
}

const GLCchar* glcGetFontListc(GLint inFont, GLCenum inAttrib, GLint inIndex)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	if (inAttrib == GLC_CHAR_LIST) {
	    if (FT_Get_Glyph_Name(font->face, inIndex, __glcBuffer, GLC_STRING_CHUNK)) {
		__glcRaiseError(GLC_INTERNAL_ERROR);
		return GLC_NONE;
	    }
	    return (GLCchar *)__glcBuffer;
	}
	else
	    return glcGetMasterListc(font->parent->id, inAttrib, inIndex);
    else
	return GLC_NONE;
}

const GLCchar* glcGetFontMap(GLint inFont, GLint inCode)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    FT_UInt glyphIndex = 0;
    datum key, content;
    
    if (font) {
	if (font->charMapCount) {
	    GLint i = 0;
	    
	    /* FIXME : use a dichotomic algo instead */
	    for (i = 0; i < font->charMapCount; i++) {
		if (font->charMap[0][i] == inCode) {
		    inCode = font->charMap[1][i];
		    break;
		}
	    }
	}
	glyphIndex = FT_Get_Char_Index(font->face, inCode);
	if (!glyphIndex)
	    return GLC_NONE;
	/* Uses GDBM to retrieve the map name */
	key.dsize = sizeof(GLint);
	key.dptr = (char *)&inCode;
	content = gdbm_fetch(unicod1, key);
	if (!content.dptr) {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    return GLC_NONE;
	}
	/* GDBM does not free the content data, so we copy the result to
	  __glcBuffer then we free the content data */
	strncpy(__glcBuffer, content.dptr, 256);
	free(content.dptr);
	return (GLCchar *)__glcBuffer;
    }
    else
	return GLC_NONE;
}

const GLCchar* glcGetFontc(GLint inFont, GLCenum inAttrib)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	return glcGetMasterc(font->parent->id, inAttrib);
    else
	return GLC_NONE;
}

GLint glcGetFonti(GLint inFont, GLCenum inAttrib)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	return glcGetMasteri(font->parent->id, inAttrib);
    else
	return 0;
}

GLboolean glcIsFont(GLint inFont)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    return (font ? GL_TRUE : GL_FALSE);
}

static GLint __glcNewFontFromMaster(GLint inFont, GLint inMaster, __glcContextState *inState)
{
    __glcFont *font = NULL;
    char buffer[256];

    if ((inMaster < 0) || (inMaster >= inState->masterCount)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    font = inState->fontList[inFont - 1];
    if (font) {
	__glcDeleteFont(inFont, inState);
	FT_Done_Face(font->face);
    }
    else {
	font = (__glcFont *)malloc(sizeof(__glcFont));
	font->id = inFont;
	inState->fontList[inFont - 1] = font;
	inState->fontCount++;
    }

    font->faceID = 0;
    font->parent = inState->masterList[inMaster];
    font->charMapCount = 0;

    if (FT_New_Face(library, __glcStringListExtractElement(&font->parent->faceFileName,
		0, buffer, 256), 0, &font->face)) {
	/* Unable to load the face file, however this should not happen since
	   it has been succesfully loaded when the master was created */
	__glcRaiseError(GLC_INTERNAL_ERROR);
	return 0;
    }
    
    /* select a Unicode charmap */
    if (FT_Select_Charmap(font->face, ft_encoding_unicode)) {
	/* Arrghhh, no Unicode charmap is available. This should not happen
	   since it has been tested at master creation */
	__glcRaiseError(GLC_INTERNAL_ERROR);
	return 0;
    }
    
    return inFont;
}

GLint glcNewFontFromMaster(GLint inFont, GLint inMaster)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    return __glcNewFontFromMaster(inFont, inMaster, state);
}

GLint glcNewFontFromFamily(GLint inFont, const GLCchar* inFamily)
{
    __glcContextState *state = NULL;
    int i = 0;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    for (i = 0; i < state->masterCount; i++) {
	if (!strcmp(inFamily, state->masterList[i]->family))
	    break;
    }
    
    if (i < state->masterCount)
	return __glcNewFontFromMaster(inFont, i, state);
    else {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
}
