/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

/* This file defines the so-called "Font commands" described in chapter 3.7
 * of the GLC specs
 */

#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_LIST_H

/* Most font commands need to check that :
 *   1. The current thread owns a context state
 *   2. The font identifier 'inFont' is legal
 * This internal function does both checks and returns the pointer to the
 * __glcFont object that is identified by 'inFont'.
 */
static __glcFont* __glcVerifyFontParameters(GLint inFont)
{
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;
  __glcFont *font = NULL;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify if the font identifier is in legal bounds */
  for (node = state->fontList->head; node; node = node->next) {
    font = (__glcFont*)node->data;
    if (font->id == inFont) break;
  }

  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Returns the __glcFont object identified by inFont */
  return font;
}

/* glcAppendFont:
 *   This command appends inFont to the list GLC_CURRENT_FONT_LIST. The command
 *   raises GLC_PARAMETER_ERROR if inFont is an element in the list
 *   GLC_CURRENT_FONT_LIST at the beginning of command execution.
 */
void glcAppendFont(GLint inFont)
{
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;
  __glcFont *font = NULL;

  /* Verify that the font exists */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  /* Check if inFont is already an element of GLC_CURRENT_FONT_LIST */
  if (FT_List_Find(state->currentFontList, font))
    return;

  node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
  if (!node) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  node->data = font;
  FT_List_Add(state->currentFontList, node);

  return;
}

/* This internal function must be called each time that a command destroys
 * a font. __glcDeleteFont removes, if necessary,  the font identified by
 * inFont from the list GLC_CURRENT_FONT_LIST.
 */
static void __glcDeleteFont(__glcFont* font, __glcContextState* state)
{
  FT_ListNode node = NULL;
    
  /* Look for the font into GLC_CURRENT_FONT_LIST */
  node = FT_List_Find(state->currentFontList, font);

  /* If the font has been found, remove it from the list */
  if (node) {
    FT_List_Remove(state->currentFontList, node);
    __glcFree(node);
  }
}

/* glcDeleteFont:
 *   This command deletes the font identified by inFont. If inFont is an
 *   element in the list GLC_CURRENT_FONT_LIST, the command removes that
 *   element from the list.
 */
void glcDeleteFont(GLint inFont)
{
  __glcContextState *state = __glcGetCurrent();
  __glcFont *font = __glcVerifyFontParameters(inFont);
  FT_ListNode node = NULL;

  /* Check if the font identified by inFont exists */
  if (!font)
    return;

  __glcDeleteFont(font, state);

  /* Destroy the font and remove it from the GLC_FONT_LIST */
  node = FT_List_Find(state->fontList, font);
  FT_List_Remove(state->fontList, node);
  __glcFree(node);
  __glcFontDestroy(font);
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

  if (inFont) {
    __glcFont* font = __glcVerifyFontParameters(inFont);
    FT_ListNode node = NULL;

    /* Check if the font exists */
    if (!font)
      return;

    state = __glcGetCurrent();

    /* Append the font identified by inFont to GLC_CURRENT_FONT_LIST */

    node = state->currentFontList->head;
    if (node) {
      /* Remove the first node of the list in order to prevent it to be
       * deleted by FT_List_Finalize().
       */
      FT_List_Remove(state->currentFontList, node);
      FT_List_Finalize(state->currentFontList, NULL,
                     __glcCommonArea->memoryManager, NULL);

      /* Reset the list */
      state->currentFontList->head = NULL;
      state->currentFontList->tail = NULL;
    }
    else {
      /* The list is empty, create a new node */
      node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
      if (!node) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
        return;
      }
    }

    /* Insert the updated node as the first and only node */
    node->data = font;
    FT_List_Add(state->currentFontList, node);
  }
  else {
    /* Verify if the current thread owns a context state */
    state = __glcGetCurrent();
    if (!state) {
      __glcRaiseError(GLC_STATE_ERROR);
      return;
    }

    /* Empties the list GLC_CURRENT_FONT_LIST */
    FT_List_Finalize(state->currentFontList, NULL,
                     __glcCommonArea->memoryManager, NULL);
    state->currentFontList->head = NULL;
    state->currentFontList->tail = NULL;
  }
}

/* This internal function tries to open the face file which name is identified
 * by inFace. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __glcFont object identified by inFont. Otherwise,
 * it leaves the font 'inFont' unchanged. GL_TRUE or GL_FALSE are returned
 * to indicate if the function succeed or not.
 */
static GLboolean __glcFontFace(__glcFont* font, const GLCchar* inFace, __glcContextState *inState)
{
  GLint faceID = 0;
  __glcUniChar UinFace;
  __glcUniChar *s = NULL;
  GLCchar* buffer = NULL;
  FT_Face newFace = NULL;

  UinFace.ptr = (GLCchar*)inFace;
  UinFace.type =  inState->stringType;

  /* Get the face ID of the face identified by the string inFace */
  faceID = __glcStrLstGetIndex(font->parent->faceList, &UinFace);
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
  s = __glcStrLstFindIndex(font->parent->faceFileName, faceID);
  buffer = __glcCtxQueryBuffer(inState, __glcUniLenBytes(s));
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
  __glcUniDup(s, buffer, __glcUniLenBytes(s));

  /* Open the new face */
  if (FT_New_Face(inState->library, 
		  (const char*)buffer, 0, &newFace)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
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
  __glcContextState *state = __glcGetCurrent();

  if (inFont) {
    __glcFont *font = __glcVerifyFontParameters(inFont);

    /* Check if the current thread owns a context state
     * and if the font identified by inFont exists
     */
    if (!font)
      return GL_FALSE;

    /* Open the new face */
    return __glcFontFace(font, inFace, state);
  }
  else {
    FT_ListNode node = NULL;

    /* Check if the current thread owns a context state */
    if (!state) {
      __glcRaiseError(GLC_STATE_ERROR);
      return GL_FALSE;
    }

    /* Search for a face which name is 'inFace' in the GLC_CURRENT_FONT_LIST */
    for (node = state->currentFontList->head; node; node = node->next) {
      if (__glcFontFace((__glcFont*)node->data, inFace, state))
	return GL_TRUE; /* A face has been found and opened */
    }
    /* No face identified by inFace has been found in GLC_CURRENT_FONT_LIST */
    return GL_FALSE;
  }
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
  __glcContextState *state = __glcGetCurrent();
  
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
    __glcUniChar UinCharName;
    GLCchar* buffer = NULL;
    int length = 0;

    UinCharName.ptr = (GLCchar*)inCharName;
    UinCharName.type = state->stringType;

    /* Convert the character name identified by inCharName into the GLC_UCS1
     * format. The result is stored into 'buffer'. */
    length = __glcUniEstimate(&UinCharName, GLC_UCS1);
    buffer = __glcCtxQueryBuffer(state, length);
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
    __glcUniConvert(&UinCharName, buffer, GLC_UCS1, length);

    /* Verify that the glyph exists in the face */
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    if (!glyphIndex) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return;
    }

    /* Use the NDBM database to retrieve the Unicode code from its name */
    code = __glcCodeFromName(buffer);
    if (code == -1) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return;
    }

    /* Check if the character identified by 'inCharName' is an element of the
     * list GLC_CHAR_LIST
     */
    if (!FT_List_Find(font->parent->charList, &code)) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
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
	__glcRaiseError(GLC_RESOURCE_ERROR);
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
  FT_ListNode node = NULL;

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Look for the last entry in the list GLC_FONT_LIST */
  node = state->fontList->tail;
  if (!node)
    return 1;
  else
    return ((__glcFont*)node->data)->id + 1;
}

/* glcGetFontFace:
 *   This command returns the string name of the current face of the font
 *   identified by inFont.
 */
const GLCchar* glcGetFontFace(GLint inFont)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);
  __glcContextState *state = __glcGetCurrent();

  if (font) {
    __glcUniChar *s = __glcStrLstFindIndex(font->parent->faceList, 
					   font->faceID);
    GLCchar *buffer = NULL;

    /* Convert the string name of the face into the current string type */
    buffer = __glcCtxQueryBuffer(state, __glcUniLenBytes(s));
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    __glcUniDup(s, buffer, __glcUniLenBytes(s));

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

  if (font) {
    __glcUniChar s;
    GLCchar *buffer = NULL;
    int length = 0;
    __glcContextState *state = __glcGetCurrent();

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

    s.ptr = __glcNameFromCode(inCode);
    s.type = GLC_UCS1;
    if (!s.ptr) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }

    /* Convert the Unicode to the current string type */
    length = __glcUniEstimate(&s, state->stringType);
    buffer = __glcCtxQueryBuffer(state, length);
    if (!buffer) {
      /* __glcFree(content.dptr); */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    __glcUniConvert(&s, buffer, state->stringType, length);

    /* Free the place allocated by NDBM and return the result */
    /* __glcFree(content.dptr); */
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
static GLint __glcNewFontFromMaster(GLint inFont, __glcMaster* inMaster, __glcContextState *inState)
{
  FT_ListNode node = NULL;
  __glcFont *font = NULL;

  for (node = inState->fontList->head; node; node = node->next) {
    font = (__glcFont*)node->data;
    if (font->id == inFont)
      break;
    else
      font = NULL;
  }

  node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
  if (!node) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  if (font) {
    /* Delete the font */
    __glcDeleteFont(font, inState);
    __glcFontDestroy(font);
  }

  /* Create a new font and add it to the list GLC_FONT_LIST */
  font = __glcFontCreate(inFont, inMaster);
  node->data = font;
  FT_List_Add(inState->fontList, node);

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
    FT_ListNode node = NULL;
    __glcMaster* master = NULL;

    /* Check if inFont is in legal bounds */
    if (inFont < 1) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    /* Check if the current thread owns a context state */
    state = __glcGetCurrent();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    /* Check if inMaster is in legal bounds */
    if (inMaster < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }

    for(node = state->masterList->head; node; node = node->next) {
      master = (__glcMaster*)node->data;
      if (master->id == inMaster) break;
    }

    /* The master identified by inMaster has not been found */
    if (!node) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return 0;
    }

    /* Create and return the new font */
    return __glcNewFontFromMaster(inFont, master, state);
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
  FT_ListNode node = NULL;
  __glcMaster* master = NULL;

  /* Check if inFont is in legal bounds */
  if (inFont < 1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Verify if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Search for a master which string attribute GLC_FAMILY is inFamily */
  for (node = state->masterList->head; node; node = node->next) {
    __glcUniChar UinFamily;

    master = (__glcMaster*)node->data;

    UinFamily.ptr = (GLCchar*)inFamily;
    UinFamily.type = state->stringType;
    if (!__glcUniCompare(&UinFamily, master->family))
      break;
  }
 
  if (node)
    /* A master has been found, create a new font and add it to the list
     * GLC_FONT_LIST */
    return __glcNewFontFromMaster(inFont, master, state);
  else {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
}
