/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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

/** \file 
 *  defines the so-called "Font commands" described in chapter 3.7 of the GLC specs
 */

/** \defgroup font Font commands
 *  A font is a stylistically consistent set of glyphs that can be used to
 *  render some set of characters. Each font has a family name (for example
 *  Palatino) and a state variable that selects one of the faces (for
 *  example Regular, Bold, Italic, BoldItalic) that the font contains. A
 *  typeface is the combination of a family and a face (for example
 *  Palatino Bold).
 *
 *  A font is an instantiation of a master.
 *
 *  Most of the commands in this category have a parameter \e inFont. Unless
 *  otherwise specified, these commands raise \b GLC_PARAMETER_ERROR if
 *  \e inFont is less than zero or is greater than or equal to the value of
 *  the variable \b GLC_FONT_COUNT.
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



/** \ingroup font
 *  This command appends \e inFont to the list \b GLC_CURRENT_FONT_LIST. The command
 *  raises \b GLC_PARAMETER_ERROR if \e inFont is an element in the list
 *  \b GLC_CURRENT_FONT_LIST at the beginning of command execution.
 *  \param inFont The ID of the font to append to the list \b GLC_CURRENT_FONT_LIST
 *  \sa glcGetListc() with argument \b GLC_CURRENT_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_CURRENT_FONT_COUNT
 *  \sa glcFont()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
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

  /* verify that the context exists */
  state = __glcGetCurrent();

  if (!state) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if inFont is already an element of GLC_CURRENT_FONT_LIST */
  if (FT_List_Find(state->currentFontList, font)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

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



/** \ingroup font
 *  This command deletes the font identified by \e inFont. If \e inFont is an
 *  element in the list \b GLC_CURRENT_FONT_LIST, the command removes that
 *  element from the list.
 *  \param inFont The ID of the font to delete
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
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



/** \ingroup font
 *  This command begins by removing all elements from the list
 *  \b GLC_CURRENT_FONT_LIST. If \e inFont is nonzero, the command then appends
 *  \e inFont to the list. Otherwise, the command does not raise an error and
 *  the list remains empty.
 *  \param inFont The ID of a font.
 *  \sa glcGetListc() with argument \b GLC_CURRENT_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_CURRENT_FONT_COUNT
 *  \sa glcAppendFont()
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

    /* There is no need to check if the following state is not NULL since
     * it has already been done by __glcVerifyFontParameters().
     */
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
 * to indicate if the function succeeded or not.
 */
static GLboolean __glcFontFace(__glcFont* font, const GLCchar* inFace,
			       __glcContextState *inState)
{
  GLint faceID = 0;
  __glcUniChar UinFace;
  __glcUniChar *s = NULL;
  GLCchar* buffer = NULL;
  FT_Face newFace = NULL;

  UinFace.ptr = (GLCchar*)inFace;
  UinFace.type = inState->stringType;

  /* Get the face ID of the face identified by the string inFace */
  faceID = __glcStrLstGetIndex(font->parent->faceList, &UinFace);
  if (faceID == -1)
    return GL_FALSE;

  /* Get the file name of the face */
  s = __glcStrLstFindIndex(font->parent->faceFileName, faceID);
  buffer = s->ptr;

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



/** \ingroup font
 *  This command attempts to set the current face of the font identified by
 *  \e inFont to the face identified by the string \e inFace. Examples for
 *  font faces are strings like <tt>"Normal"</tt>, <tt>"Bold"</tt> or
 *  <tt>"Bold Italic"</tt>. In contrast to some systems that have a different
 *  font for each face, GLC allows you to have the face be an attribute of
 *  the font.
 *
 *  If \e inFace is not an element of the font's string list attribute
 *  \b GLC_FACE_LIST, the command leaves the font's current face unchanged and
 *  returns \b GL_FALSE. If the command suceeds, it returns \b GL_TRUE.
 *
 *  If \e inFont is zero, the command iterates over the \b GLC_CURRENT_FONT_LIST.
 *  For each of the fonts named therein, the command attempts to set the
 *  font's current face to the face in that font that is identified by \e inFace.
 *  In this case, the command returns \b GL_TRUE if \b GLC_CURRENT_FONT_LIST
 *  contains one or more elements and the command successfully sets the
 *  current face of each of the fonts named in the list.
 *  \param inFont The ID of the font to be changed
 *  \param inFace The face for \e inFont
 *  \return \b GL_TRUE if the command succeeded to set the face \e inFace
 *          to the font \e inFont. \b GL_FALSE is returned otherwise.
 *  \sa glcGetFontFace()
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



/** \ingroup font
 *  This command modifies the map of the font identified by \e inFont such that
 *  the font maps \e inCode to the character whose name is the string \e inCharName.
 *  If \e inCharName is \b GLC_NONE,\e inCode is removed from the font's map.
 *
 *  Every font has an associated font map. A font map is a table of entries that
 *  map integer values to the name string that identifies the character.
 *
 *  Every character code used in QuesoGLC is an element of the Universal
 *  Multiple-Octet Coded Character Set (UCS) defined by the standards ISO/IEC
 *  10646-1:1993 and Unicode 2.0 (unless otherwise specified). A UCS code is
 *  denoted as \e U+hexcode, where \e hexcode is a sequence of hexadecimal
 *  digits. Each UCS code corresponds to a character that has a unique name
 *  string. For example, the code \e U+41 corresponds to the character
 *  <em>LATIN CAPITAL LETTER A</em>.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inCharName is not \b GLC_NONE
 *  or an element of the font string's list attribute \b GLC_CHAR_LIST.
 *  \param inFont The ID of the font
 *  \param inCode The integer ID of a character
 *  \param inCharName The string name of a character
 *  \sa glcGetFontMap()
 */
void glcFontMap(GLint inFont, GLint inCode, const GLCchar* inCharName)
{
  __glcFont *font = NULL;
  GLint i = 0;

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
	  memmove(&font->charMap[0][i], &font->charMap[0][i+1],
		  (font->charMapCount-i-1) * 2 * sizeof(GLint));
	font->charMapCount--;
	break;
      }
    }
    return;
  }
  else {
    FT_UInt glyphIndex = 0;
    FT_ULong code  = 0;
    __glcContextState *state = __glcGetCurrent();
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

    /* Retrieve the Unicode code from its name */
    code = __glcCodeFromName(buffer);
    if (code == -1) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return;
    }

    /* Verify that the glyph exists in the face */
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    if (!glyphIndex) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return;
    }

    /* Add the character identified by 'inCharName' to the list GLC_CHAR_LIST.
     */
    if (!FcCharSetHasChar(font->parent->charList, code))
      FcCharSetAddChar(font->parent->charList, code);

    /* FIXME : use a dichotomic algo instead */
    for (i = 0; i < font->charMapCount; i++) {
      if (font->charMap[0][i] >= code)
	break;
    }
    if (font->charMap[0][i] != code) {
      /* The character identified by inCharName is not yet registered, we add
       * it to the charmap.
       */
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



/** \ingroup font
 *  This command returns a font ID that is not an element of the list
 *  \b GLC_FONT_LIST.
 *  \return a new font ID
 *  \sa glcDeleteFont()
 *  \sa glcIsFont()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
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



/** \ingroup font
 *  This command returns the string name of the current face of the font
 *  identified by \e inFont.
 *  \param inFont The font ID
 *  \return the string name of the font \e inFont
 *  \sa glcFontFace()
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



/** \ingroup font
 *  This command returns an attribute of the font identified by \e inFont that
 *  is a string from a string list identified by \e inAttrib. The command returns
 *  the string at offset \e inIndex from the first element in \e inAttrib. For
 *  example, if \e inFont has a face list (\c Regular, \c Bold, \c Italic ) and
 *  \e inIndex is \c 2, then the command returns \c Italic if you query \b GLC_FACE_LIST.
 *
 *  Every GLC state variable that is a list has an associated integer element
 *  count whose value is the number of elements in the list.
 *
 *  Below are the string list attributes associated with each GLC master and font
 *  and their element count attributes :
 * <center>
 * <table>
 * <caption>Master/font string list attributes</caption>
 *   <tr>
 *     <td>Name</td> <td>Enumerant</td> <td>Element count attribute</td>
 *   </tr>
 *   <tr>
 *     <td><b>GLC_CHAR_LIST</b></td>
 *     <td>0x0050</td>
 *     <td><b>GLC_CHAR_COUNT</b></td>
 *   </tr>
 *   <tr>
 *     <td><b>GLC_FACE_LIST</b></td>
 *     <td>0x0051</td>
 *     <td><b>GLC_FACE_COUNT</b></td>
 *   </tr>
 * </table>
 * </center>
 *  \n The command raises \b GLC_PARAMETER_ERROR if \e inIndex is less than
 *  zero or is greater than or equal to the value of the list element count
 *  attribute.
 *  \param inFont The font ID
 *  \param inAttrib The string list from which a string is requested
 *  \param inIndex The offset from the first element of the list associated
 *                 with \e inAttrib
 *  \return a string attribute of \e inFont identified by \e inAttrib
 *  \sa glcGetMasterListc()
 *  \sa glcGetFontc()
 *  \sa glcGetFonti()
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



/** \ingroup font
 *  This command returns the string name of the character that the font identified
 *  by \e inFont maps \e inCode to.
 *
 *  Every font has an associated font map. A font map is a table of entries that
 *  map integer values to the name string that identifies the character.
 *
 *  To change the map, that is, associate a different name string with the integer
 *  ID of a font, use glcFontMap().
 *
 *  If \e inCode cannot be mapped in the font, the command returns \b GLC_NONE.
 *  \note Changing the map of a font is possible but changing the map for a
 *        master is not.
 *  \param inFont The integer ID of the font from which to select the character
 *  \param inCode The integer ID of the character in the font map
 *  \return the string name of the character that the font \e inFont maps
 *          \e inCode to.
 *  \sa glcGetMasterMap()
 *  \sa glcFontMap()
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

    /* __glcFree(content.dptr); */
    return buffer;
  }
  else
    return GLC_NONE;
}



/** \ingroup font
 *  This command returns a string attribute of the font identified by
 *  \e inFont. The table below lists the string attributes that are
 *  associated with each GLC master and font.
 *  <center>
 *  <table>
 *  <caption>Master/font string attributes</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FAMILY</b></td> <td>0x0060</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MASTER_FORMAT</b></td> <td>0x0061</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_VENDOR</b></td> <td>0x0062</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_VERSION</b></td> <td>0x0063</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inFont The font for which the attribute is requested
 *  \param inAttrib The requested string attribute
 *  \return the string attribute \e inAttrib of the font \e inFont
 *  \sa glcGetMasterc()
 *  \sa glcGetFonti()
 *  \sa glcGetFontListc()
 */
const GLCchar* glcGetFontc(GLint inFont, GLCenum inAttrib)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);

  if (font)
    return glcGetMasterc(font->parent->id, inAttrib);
  else
    return GLC_NONE;
}



/** \ingroup font
 *  This command returns an integer attribute of the font identified by
 *  \e inFont. The attribute is identified by \e inAttrib. The table below
 *  lists the integer attributes that are associated with each GLC master
 *  and font.
 *  <center>
 *  <table>
 *  <caption>Master/font integer attributes</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_CHAR_COUNT</b></td> <td>0x0070</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FACE_COUNT</b></td> <td>0x0071</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_IS_FIXED_PITCH</b></td> <td>0x0072</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MAX_MAPPED_CODE</b></td> <td>0x0073</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MIN_MAPPED_CODE</b></td> <td>0x0074</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inFont The font for which the attribute is requested.
 *  \param inAttrib The requested integer attribute
 *  \return the value of the specified integer attribute
 *  \sa glcGetMasteri()
 *  \sa glcGetFontc()
 *  \sa glcGetFontListc()
 */
GLint glcGetFonti(GLint inFont, GLCenum inAttrib)
{
  __glcFont *font = __glcVerifyFontParameters(inFont);

  if (font)
    return glcGetMasteri(font->parent->id, inAttrib);
  else
    return 0;
}



/** \ingroup font
 *  This command returns \b GL_TRUE if \e inFont is the ID of a font. If
 *  \e inFont is not the ID of a font, the command does not raise an error.
 *  \param inFont The element to be tested
 *  \return \b GL_TRUE if \e inFont is the ID of a font, \b GL_FALSE
 *          otherwise.
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
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
static GLint __glcNewFontFromMaster(GLint inFont, __glcMaster* inMaster,
				    __glcContextState *inState)
{
  FT_ListNode node = NULL;
  __glcFont *font = NULL;

  for (node = inState->fontList->head; node; node = node->next) {
    font = (__glcFont*)node->data;
    if (font->id == inFont)
      break;
    else
      font = NULL; /* font will be NULL if inFont ID is not in the font list */
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



/** \ingroup font
 *  This command creates a new font from the master identified by \e inMaster.
 *  The ID of the new font is \e inFont. If the command succeeds, it returns
 *  \e inFont. If \e inFont is the ID of a font at the beginning of command
 *  execution, the command executes the command \c glcDeleteFont(inFont) before
 *  creating the new font.
 *  \param inFont The ID of the new font
 *  \param inMaster The master from which to create the new font
 *  \return The ID of the new font if the command succceeds, \c 0 otherwise.
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcDeleteFont()
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



/** \ingroup font
 *  This command performs a sequential search beginning with the first element
 *  of the GLC master list, looking for the first master whose string
 *  attribute \b GLC_FAMILY equals \e inFamily. If there is no such master the
 *  command returns zero. Otherwise, the command creates a new font from the
 *  master. The ID of the new font is \e inFont.
 *
 *  If the command succeeds, it returns \e inFont. If \e inFont is the ID of a
 *  font at the beginning of command execution, the command executes the command
 *  \c glcDeleteFont(inFont) before creating the new font.
 *  \param inFont The ID of the new font.
 *  \param inFamily The font family, that is, the string that \b GLC_FAMILY
 *                  attribute has to match.
 *  \return the ID of the new font if the command succeeds, \c 0 otherwise.
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromMaster()
 *  \sa glcDeleteFont()
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
