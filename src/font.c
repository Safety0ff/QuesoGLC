/* $Id$ */
#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"

void glcAppendFont(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    if (!glcIsFont(inFont))
	return;
    
    if (state->currentFontCount < GLC_MAX_CURRENT_FONT) {
	state->currentFontList[state->currentFontCount] = inFont;
	state->currentFontCount++;
    }
    else
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);

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

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    if (!glcIsFont(inFont))
	return;

    __glcDeleteFont(inFont, state);
    font = state->fontList[inFont - 1];
    delete font;
    state->fontList[inFont - 1] = NULL;
    state->fontCount--;
}

void glcFont(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
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
    __glcUniChar UinFace = __glcUniChar(inFace, inState->stringType);
    __glcUniChar s;
    GLCchar* buffer = NULL;

    font = inState->fontList[inFont];
    
    if (!font) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return GL_FALSE;
    }

    faceID = font->parent->faceList->getIndex(&UinFace);
    if (faceID == -1)
	return GL_FALSE;

    font->faceID = faceID;

    s = __glcUniChar(font->parent->faceFileName->findIndex(faceID), GLC_UCS1);
    buffer = inState->queryBuffer(s.lenBytes());
    if (!buffer) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return GL_FALSE;
    }
    s.dup(buffer, s.lenBytes());

    if (FT_New_Face(__glcContextState::library, 
		    (const char*)buffer, 0, &font->face)) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return GL_FALSE;
    }
    
    return GL_TRUE;
}

static __glcFont* __glcVerifyFontParameters(GLint inFont)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return NULL;
    }

    if ((inFont < 1) || (inFont >= GLC_MAX_FONT)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return NULL;
    }

    return state->fontList[inFont - 1];
}

GLboolean glcFontFace(GLint inFont, const GLCchar* inFace)
{
    __glcContextState *state = NULL;
    __glcFont *font = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
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
  __glcContextState *state = __glcContextState::getCurrent();
  
  if (font) {
    __glcUniChar UinCharName = __glcUniChar(inCharName, state->stringType);
    GLCchar* buffer = NULL;
    int length = 0;

    length = UinCharName.estimate(GLC_UCS1);
    buffer = state->queryBuffer(length);
    if (!buffer) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
    UinCharName.convert(buffer, GLC_UCS1, length);

    /* Verify that the glyph exists in the face */
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    if (!glyphIndex) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return;
    }
    /* Retrieve the Unicode code from its name */
    key.dsize = strlen((const char*)buffer) + 1;
    key.dptr = (char *)buffer;
    content = gdbm_fetch(__glcContextState::unidb2, key);
    if (!content.dptr) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
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
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }
    font->charMap[1][i] = inCode;
  }
  else {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }
}

GLint glcGenFontID(void)
{
    __glcContextState *state = NULL;
    int i = 0;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

    for (i = 0; i < GLC_MAX_FONT; i++) {
	if (!state->fontList[i])
	    break;
    }
    
    if (i < GLC_MAX_FONT)
	return i + 1;
    else {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
}

const GLCchar* glcGetFontFace(GLint inFont)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  __glcContextState *state = __glcContextState::getCurrent();

  if (font) {
    __glcUniChar *s = font->parent->faceList->findIndex(font->faceID);
    GLCchar *buffer = NULL;

    buffer = state->queryBuffer(s->lenBytes());
    if (!buffer) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    s->dup(buffer, s->lenBytes());
    return buffer;
  }
  else
    return GLC_NONE;
}

const GLCchar* glcGetFontListc(GLint inFont, GLCenum inAttrib, GLint inIndex)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  __glcContextState *state = __glcContextState::getCurrent();
  char glyphName[1024];
  __glcUniChar s =__glcUniChar(glyphName, GLC_UCS1);
    
  if (font) {
    GLCchar *buffer = NULL;
    int length = 0;

    if (inAttrib == GLC_CHAR_LIST) {
      if (FT_Get_Glyph_Name(font->face, inIndex, glyphName, GLC_STRING_CHUNK)) {
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	return GLC_NONE;
      }

      length = s.estimate(state->stringType);
      buffer = state->queryBuffer(length);
      if (!buffer) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return GLC_NONE;
      }
      s.convert(buffer, state->stringType, length);

      return (GLCchar *)buffer;
    }
    else
      return glcGetMasterListc(font->parent->id, inAttrib, inIndex);
  }
  else
    return GLC_NONE;
}

const GLCchar* glcGetFontMap(GLint inFont, GLint inCode)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  FT_UInt glyphIndex = 0;
  datum key, content;

  if (font) {
    __glcUniChar s = __glcUniChar(content.dptr, GLC_UCS1);
    GLCchar *buffer = NULL;
    int length = 0;
    __glcContextState *state = __glcContextState::getCurrent();

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
    content = gdbm_fetch(__glcContextState::unidb1, key);
    if (!content.dptr) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }

    length = s.estimate(state->stringType);
    buffer = state->queryBuffer(length);
    if (!buffer) {
      free(content.dptr);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    s.convert(buffer, state->stringType, length);

    free(content.dptr);
    return buffer;
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

    if ((inMaster < 0) || (inMaster >= inState->masterCount)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    font = inState->fontList[inFont - 1];
    if (font) {
      __glcDeleteFont(inFont, inState);
      delete font;
    }

    font = new __glcFont(inFont, inState->masterList[inMaster]);
    inState->fontList[inFont - 1] = font;
    inState->fontCount++;

    return inFont;
}

GLint glcNewFontFromMaster(GLint inFont, GLint inMaster)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

    if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    return __glcNewFontFromMaster(inFont, inMaster, state);
}

GLint glcNewFontFromFamily(GLint inFont, const GLCchar* inFamily)
{
  __glcContextState *state = NULL;
  int i = 0;

  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0;
  }

  if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  for (i = 0; i < state->masterCount; i++) {
    __glcUniChar UinFamily = __glcUniChar(inFamily, state->stringType);
    if (!UinFamily.compare(state->masterList[i]->family))
      break;
  }
 
  if (i < state->masterCount)
    return __glcNewFontFromMaster(inFont, i, state);
  else {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
}
