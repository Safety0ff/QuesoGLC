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
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_LIST_H

/* glcAppendFont:
 *   This command appends inFont to the list GLC_CURRENT_FONT_LIST. The command
 *   raises GLC_PARAMETER_ERROR if inFont is an element in the list
 *   GLC_CURRENT_FONT_LIST at the beginning of command execution.
 */
void glcAppendFont(GLint inFont)
{
  __glcContextState *state = NULL;
  GLint i = 0;

  /* Verify that the font exists */
  if (!glcIsFont(inFont))
    return;

  /* Check if the thread as a current context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  /* Check if inFont is already an element of GLC_CURRENT_FONT_LIST */
  for (i = 0; i < state->currentFontCount; i++) {
    if (state->currentFontList[i] == inFont) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return;
    }
  }

  if (state->currentFontCount < GLC_MAX_CURRENT_FONT) {
    /* Add the font to the GLC_CURRENT_FONT_LIST */
    state->currentFontList[state->currentFontCount] = inFont;
    state->currentFontCount++;
  }
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return;
}

/* This internal function must be called each time that a command destroys
 * a font. __glcDeleteFont removes, if necessary,  the font identified by
 * inFont from the list GLC_CURRENT_FONT_LIST.
 */
static void __glcDeleteFont(GLint inFont, __glcContextState* state)
{
  int i = 0;
    
  /* Look for the font into GLC_CURRENT_FONT_LIST */
  for(i = 0; i < state->currentFontCount; i++) {
    if (state->currentFontList[i] == inFont)
      break;
  }
 
  /* If the font has been found, remove it from the list */
  if (i < state->currentFontCount) {
    memmove(&state->currentFontList[i], &state->currentFontList[i+1],
	    (GLC_MAX_CURRENT_FONT - i -1) * sizeof(GLint));
    state->currentFontCount--;
  }
}

/* glcDeleteFont:
 *   This command deletes the font identified by inFont. If inFont is an
 *   element in the list GLC_CURRENT_FONT_LIST, the command removes that
 *   element from the list.
 */
void glcDeleteFont(GLint inFont)
{
  __glcContextState *state = NULL;
  __glcFont *font = NULL;

  /* Check if the font identified by inFont exists */
  if (!glcIsFont(inFont))
    return;

  /* Verify that the current thread owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  __glcDeleteFont(inFont, state);

  /* Destroy the font and remove it from the GLC_FONT_LIST */
  font = state->fontList[inFont - 1];
  delete font;
  state->fontList[inFont - 1] = NULL;
  state->fontCount--;
}

/* glcFont:
 *   This command begins by removing all elements from the list
 *   GLC_CURRENT_FONT_LIST. If inFont is nonzero, the command then appends
 *   inFont to the list. Otherwise, the command does not raise an error and
 *   the list remains empty.
 */
void glcFont(GLint inFont)
{
  __glcContextState *state = NULL;

  /* If inFont is nonzero, check if it exists */
  if (inFont)
    if (!glcIsFont(inFont))
      return;

  /* Verify if the current thead owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  /* Empties the list GLC_CURRENT_FONT_LIST */
  state->currentFontCount = 0;

  /* If inFont is nonzero then append it to GLC_CURRENT_FONT_LIST */
  if (inFont) {
    state->currentFontList[0] = inFont;
    state->currentFontCount = 1;
  }
}

/* This internal function tries to open the face file which name is identified
 * by inFace. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __glcFont object identified by inFont. Otherwise,
 * it leaves the font 'inFont' unchanged. GL_TRUE or GL_FALSE are returned
 * to indicate if the function succeed or not.
 */
static GLboolean __glcFontFace(GLint inFont, const GLCchar* inFace, __glcContextState *inState)
{
  __glcFont *font = NULL;
  GLint faceID = 0;
  __glcUniChar UinFace = __glcUniChar(inFace, inState->stringType);
  __glcUniChar *s = NULL;
  GLCchar* buffer = NULL;
  FT_Face newFace = NULL;

  /* Check if the font identified by inFont exists */
  font = inState->fontList[inFont];
  if (!font) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GL_FALSE;
  }

  /* Get the face ID of the face identified by the string inFace */
  faceID = font->parent->faceList->getIndex(&UinFace);
  if (faceID == -1)
    return GL_FALSE;

  /* Get the file name of the face and copy it into a buffer */
  /* NOTE : This operation is actually overkill since no conversion
   *        is done during the process. We have to do it this way
   *        because of data encapsulation (namely of the pointer 'ptr'
   *        of the Unicode string object __glcUniChar). This is to be
   *        removed in the near future but is kept for now in order to
   *        prevent illegal accesses to __glcUniChar's 'ptr'.
   */
  s = font->parent->faceFileName->findIndex(faceID);
  buffer = inState->queryBuffer(s->lenBytes());
  if (!buffer) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
  s->dup(buffer, s->lenBytes());

  /* Open the new face */
  if (FT_New_Face(inState->library, 
		  (const char*)buffer, 0, &newFace)) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  /* If we get there then the face has been successfully opened. We close
   * the current face (if any) and store the new face's attributes in the
   * relevant font.
   */
  if (font->face) {
    FT_Done_Face(font->face);
    font->face = NULL;
  }

  font->faceID = faceID;
  font->face = newFace;

  return GL_TRUE;
}

/* glcFontFace:
 *   This command attempts to set the current face of the font identified by
 *   inFont to the face identified by the string inFace. If inFace is not an
 *   element of the font's string list attribute GLC_FACE_LIST, the command
 *   leaves the font's current face unchanged and returns GL_FALSE. If the
 *   command suceeds, it returns GL_TRUE.
 *   If inFont is zero, the command iterates over the GLC_CURRENT_FONT_LIST.
 *   For each of the fonts named therein, the command attempts to set the
 *   font's current face to the face in that font that is identified by inFace.
 *   In this case, the command returns GL_TRUE if GLC_CURRENT_FONT_LIST
 *   contains one or more elements and the command successfully sets the
 *   current face of each of the fonts named in the list.
 */
GLboolean glcFontFace(GLint inFont, const GLCchar* inFace)
{
  __glcContextState *state = NULL;
  __glcFont *font = NULL;

  /* Check if the font identified by inFont exists */
  if (!glcIsFont(inFont))
    return GL_FALSE;

  /* Check if the current thread owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GL_FALSE;
  }

  /* Get the __glcFont object identified by inFont */
  font = state->fontList[inFont - 1];
  
  if (font)
    /* Open the new face */
    return __glcFontFace(inFont - 1, inFace, state);
  else {
    int i = 0;

    /* Search for a face which name is 'inFace' in the GLC_CURRENT_FONT_LIST */
    for(i = 0; i < state->currentFontCount; i++) {
      if (__glcFontFace(state->currentFontList[i] - 1, inFace, state))
	return GL_TRUE; /* A face has been found and opened */
    }
    /* No face identified by inFace has been found in GLC_CURRENT_FONT_LIST */
    return GL_FALSE;
  }
}

/* Most font commands need to check that :
 *   1. The current thread owns a context state
 *   2. The font identifier 'inFont' is legal
 * This internal function does both checks and returns the pointer to the
 * __glcFont object that is identified by 'inFont'.
 */
static __glcFont* __glcVerifyFontParameters(GLint inFont)
{
  __glcContextState *state = NULL;

  /* Check if the current thread owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify if the font identifier is in legal bounds */
  if ((inFont < 1) || (inFont >= GLC_MAX_FONT)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Returns the __glcFont object identified by inFont */
  return state->fontList[inFont - 1];
}

/* glcFontMap:
 *   This command modifies the map of the font identified by inFont such that
 *   the font maps inCode to the character whose name is the string inCharName.
 *   If inCharName is GLC_NONE, inCode is removed from the font's map. The
 *   command raises GLC_PARAMETER_ERROR if inCharName is not GLC_NONE or an
 *   element of the font string's list attribute GLC_CHAR_LIST.
 */
void glcFontMap(GLint inFont, GLint inCode, const GLCchar* inCharName)
{
  __glcFont *font = NULL;
  FT_UInt glyphIndex = 0;
  GLint i = 0;
  FT_ULong code  = 0;
  datum key, content;
  __glcContextState *state = __glcContextState::getCurrent();
  
  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  if (!inCharName) {
    /* Look for the character mapped by inCode in the charmap */
    /* FIXME : use a dichotomic algo. instead */
    for (i = 0; i < font->charMapCount; i++) {
      if (font->charMap[1][i] == (FT_ULong)inCode) {
	/* Remove the character mapped by inCode */
	if (i < font->charMapCount-1)
	  memmove(&font->charMap[0][i], &font->charMap[0][i+1], (font->charMapCount-i-1) * 2 * sizeof(GLint));
	font->charMapCount--;
	break;
      }
    }
    return;
  }
  else {
    __glcUniChar UinCharName = __glcUniChar(inCharName, state->stringType);
    GLCchar* buffer = NULL;
    int length = 0;

    /* Convert the character name identified by inCharName into the GLC_UCS1
     * format. The result is stored into 'buffer'. */
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

    /* Use the GDBM database to retrieve the Unicode code from its name */
    key.dsize = strlen((const char*)buffer) + 1;
    key.dptr = (char *)buffer;
    content = gdbm_fetch(__glcContextState::unidb2, key);
    if (!content.dptr) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
    code = (GLint)(*(content.dptr));
    __glcFree(content.dptr);

    /* Check if the character identified by 'inCharName' is an element of the
     * list GLC_CHAR_LIST
     */
    if (!FT_List_Find(font->parent->charList, &code)) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return;
    }

    /* FIXME : use a dichotomic algo instead */
    for (i = 0; i < font->charMapCount; i++) {
      if (font->charMap[0][i] >= code)
	break;
    }
    if (font->charMap[0][i] != code) {
      /* The character identified by inCharName is not yet registered, we add
       * it to the charmap */
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
    /* Stores the code which 'inCharName' must be mapped by */
    font->charMap[1][i] = inCode;
  }
}

/* glcGenFontID:
 *   This command returns a font ID that is not an element of the list
 *   GLC_FONT_LIST
 */
GLint glcGenFontID(void)
{
  __glcContextState *state = NULL;
  int i = 0;

  /* Verify if the current thread owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Look for the first free entry in the list GLC_FONT_LIST */
  for (i = 0; i < GLC_MAX_FONT; i++) {
    if (!state->fontList[i])
      break;
  }

  /* Returns its ID if it is in legal bounds, otherwise generate an error */
  if (i < GLC_MAX_FONT)
    return i + 1;
  else {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
}

/* glcGetFontFace:
 *   This command returns the string name of the current face of the font
 *   identified by inFont.
 */
const GLCchar* glcGetFontFace(GLint inFont)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  __glcContextState *state = __glcContextState::getCurrent();

  if (font) {
    __glcUniChar *s = font->parent->faceList->findIndex(font->faceID);
    GLCchar *buffer = NULL;

    /* Convert the string name of the face into the current string type */
    buffer = state->queryBuffer(s->lenBytes());
    if (!buffer) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    s->dup(buffer, s->lenBytes());

    /* returns the name */
    return buffer;
  }
  else
    return GLC_NONE;
}

/* glcGetFontListc:
 *   This command is identical to the command glcGetMasterListc, except that
 *   it operates on a font (identified by inFont), not a master.
 */
const GLCchar* glcGetFontListc(GLint inFont, GLCenum inAttrib, GLint inIndex)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);

  if (font) {
    if (inAttrib == GLC_CHAR_LIST) {
      FT_UInt glyphIndex = 0;

      /* Check if the character exists in the font */
      /* NOTE : should we check directly inIndex or should we use the
       *        charmap ?
       */
      glyphIndex = FT_Get_Char_Index(font->face, inIndex);
      if (!glyphIndex)
	return GLC_NONE;
    }
    return glcGetMasterListc(font->parent->id, inAttrib, inIndex);
  }
  else
    return GLC_NONE;
}

/* glcGetFontMap:
 *   This command is identical to the command glcGetMasterMap, except that it
 *   operates on a font (identified by inFont), not a master.
 */
const GLCchar* glcGetFontMap(GLint inFont, GLint inCode)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  FT_UInt glyphIndex = 0;
  datum key, content;

  if (font) {
    __glcUniChar s;
    GLCchar *buffer = NULL;
    int length = 0;
    __glcContextState *state = __glcContextState::getCurrent();

    if (font->charMapCount) {
      GLint i = 0;
    
      /* Look for the character which the character identifed by inCode is
       * mapped by */
      /* FIXME : use a dichotomic algo instead */
      for (i = 0; i < font->charMapCount; i++) {
	if (font->charMap[0][i] == (FT_ULong)inCode) {
	  inCode = font->charMap[1][i];
	  break;
	}
      }
    }

    /* Check if the character identified by inCode exists in the font */
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    if (!glyphIndex)
      return GLC_NONE;

    /* Uses GDBM to retrieve the Unicode name of the character */
    key.dsize = sizeof(GLint);
    key.dptr = (char *)&inCode;
    content = gdbm_fetch(__glcContextState::unidb1, key);
    if (!content.dptr) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    s = __glcUniChar(content.dptr, GLC_UCS1);

    /* Convert the Unicode to the current string type */
    length = s.estimate(state->stringType);
    buffer = state->queryBuffer(length);
    if (!buffer) {
      __glcFree(content.dptr);
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    s.convert(buffer, state->stringType, length);

    /* Free the place allocated by GDBM and return the result */
    __glcFree(content.dptr);
    return buffer;
  }
  else
    return GLC_NONE;
}

/* glcGetFontc:
 *   This command is identical to the command glcGetMasterc, except that it
 *   operates on a font (identified by inFont), not a master
 */
const GLCchar* glcGetFontc(GLint inFont, GLCenum inAttrib)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	return glcGetMasterc(font->parent->id, inAttrib);
    else
	return GLC_NONE;
}

/* glcGetFonti:
 *   This command is identical to the command glcGetMasteri, except that it
 *   operates on a font (identified by inFont), not a master
 */
GLint glcGetFonti(GLint inFont, GLCenum inAttrib)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    if (font)
	return glcGetMasteri(font->parent->id, inAttrib);
    else
	return 0;
}

/* glcIsFont:
 *   This command returns GL_TRUE if inFont is the ID of a font. If inFont is
 *   not the ID of a font, the command does not raise an error.
 */
GLboolean glcIsFont(GLint inFont)
{
    __glcFont *font = __glcVerifyFontParameters(inFont);
    
    return (font ? GL_TRUE : GL_FALSE);
}

/* This internal function checks the validity of the parameter inMaster,
 * deletes the font identified by inFont (if any) and create a new font which
 * is added to the list GLC_FONT_LIST.
 */
static GLint __glcNewFontFromMaster(GLint inFont, GLint inMaster, __glcContextState *inState)
{
    __glcFont *font = NULL;

    /* Check if inMaster is in legal bounds */
    if ((inMaster < 0) || (inMaster >= inState->masterCount)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    /* Get the font identified by inFont */
    font = inState->fontList[inFont - 1];
    if (font) {
      /* Delete the font */
      __glcDeleteFont(inFont, inState);
      delete font;
    }

    /* Create a new font and add it to the list GLC_FONT_LIST */
    font = new __glcFont(inFont, inState->masterList[inMaster]);
    inState->fontList[inFont - 1] = font;
    inState->fontCount++;

    return inFont;
}

/* glcNewFontFromMaster:
 *   This command creates a new font from the master identified by inMaster.
 *   The ID of the new font is inFont. If the command succeeds, it returns
 *   inFont. If inFont is the ID of a font at the beginning of command
 *   execution, the command executes the command glcDeleteFont(inFont) before
 *   creating the new font.
 */
GLint glcNewFontFromMaster(GLint inFont, GLint inMaster)
{
    __glcContextState *state = NULL;

    /* Check if inFont is in legal bounds */
    if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    /* Check if the current thread owns a context state */
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

    /* Create and return the new font */
    return __glcNewFontFromMaster(inFont, inMaster, state);
}

/* glcNewFontFromFamily:
 *   This command performs a sequential search beginning with the first element
 *   of the GLC master list, looking for the first master whose string
 *   attribute GLC_FAMILY equals inFamily. If there is no such master the
 *   command returns zero. Otherwise, the command creates a new font from the
 *   master. The ID of the new font is inFont. If the command succeeds, it
 *   returns inFont. If inFont is the ID of a font at the beginning of command
 *   execution, the command executes the command glcDeleteFont(inFont) before
 *   creating the new font.
 */
GLint glcNewFontFromFamily(GLint inFont, const GLCchar* inFamily)
{
  __glcContextState *state = NULL;
  int i = 0;

  /* Check if inFont is in legal bounds */
  if ((inFont < 1) || (inFont > GLC_MAX_FONT)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Verify if the current thread owns a context state */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Search for a master which string attribute GLC_FAMILY is inFamily */
  for (i = 0; i < state->masterCount; i++) {
    __glcUniChar UinFamily = __glcUniChar(inFamily, state->stringType);
    if (!UinFamily.compare(state->masterList[i]->family))
      break;
  }
 
  if (i < state->masterCount)
    /* A master has been found, create a new font and add it to the list
     * GLC_FONT_LIST */
    return __glcNewFontFromMaster(inFont, i, state);
  else {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
}
