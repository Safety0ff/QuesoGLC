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

/** \file
 * defines the so-called "Master commands" described in chapter 3.6 of the GLC
 * specs.
 */

/** \defgroup master Master Commands
 *  A master is a representation of a font that is stored outside QuesoGLC in a
 *  standard format such as TrueType or Type1.
 *
 *  Some GLC commands have a parameter \e inMaster. This parameter is an offset
 *  from the the first element in the GLC master list. The command raises
 *  \b GLC_PARAMETER_ERROR if \e inMaster is less than zero or is greater than
 *  or equal to the value of the variable \b GLC_MASTER_COUNT.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_LIST_H



/** \ingroup master
 *  This command returns a string from a string list that is an attribute of
 *  the master identified by \e inMaster. The string list is identified by
 *  \e inAttrib. The command returns the string at offset \e inIndex from the
 *  first element in this string list. Below are the string list attributes
 *  associated with each GLC master and font and their element count
 *  attributes :
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
 *  \param inMaster Master from which an attribute is needed.
 *  \param inAttrib String list that contains the desired attribute.
 *  \param inIndex Offset from the first element of the list associated with
 *                 \e inAttrib.
 *  \return The string at offset \e inIndex from the first element of the
 *          string list identified by \e inAttrib.
 *  \sa glcGetMasterMap()
 *  \sa glcGetMasterc()
 *  \sa glcGetMasteri()
 */
const GLCchar* glcGetMasterListc(GLint inMaster, GLCenum inAttrib,
				 GLint inIndex)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar* s = NULL;
  GLCchar *buffer = NULL;
  GLint length = 0;
  FT_ListNode node = NULL;

  /* Check some parameter.
   * NOTE : the verification of some parameters needs to get the current
   *        context state but since we are supposed to check parameters
   *        _before_ the context state, we are done !
   */
  switch(inAttrib) {
  case GLC_CHAR_LIST:
  case GLC_FACE_LIST:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify if inIndex and inMaster are in legal bounds */
  if ((inIndex < 0) || (inMaster < 0)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify if a context is current to the thread */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  for(node = state->masterList->head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    if (master->id == inMaster) break;
  }

  /* No master identified by inMaster has been found */
  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  switch(inAttrib) {
  case GLC_CHAR_LIST:
    for (node = master->charList->head; inIndex && node;
	 node = node->next, inIndex--) {}

    if (node)
      return glcGetMasterMap(inMaster, (FT_ULong)node->data);
    else {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
  case GLC_FACE_LIST:
    /* Get the face name */
    s = __glcStrLstFindIndex(master->faceList, inIndex);
    if (!s) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    break;
  }

  /* Allocate a buffer in order to store the string in the current string
   * type.
   */
  length = __glcUniEstimate(s, state->stringType);
  buffer = __glcCtxQueryBuffer(state, length);
  if (buffer)
    __glcUniConvert(s, buffer, state->stringType, length);
  else
    __glcRaiseError(GLC_RESOURCE_ERROR);

  return buffer;
}



/** \ingroup master
 *  This command returns the string name of the character that the master
 *  identified by \e inMaster maps \e inCode to.
 *
 *  Every master has associated with it a master map, which is a table of
 *  entries that map inetger values to the name string that identifies the
 *  character.
 *
 *  Every character code used in QuesoGLC is an element of the Universal
 *  Multiple-Octet Coded Character Set (UCS) defined by the standards ISO/IEC
 *  10646-1:1993 and Unicode 2.0 (unless otherwise specified). A UCS code is
 *  denoted as \e U+hexcode, where \e hexcode is a sequence of hexadecimal
 *  digits. Each UCS code corresponds to a character that has a unique name
 *  string. For example, the code \e U+41 corresponds to the character
 *  <em>LATIN CAPITAL LETTER A</em>.
 *
 *  If the master does not map \e inCode, the command returns \b GLC_NONE.
 *  \note While you cannot change the map for a master, you can change the map
 *  for a font using glcFontMap().
 *  \param inMaster The integer ID of the master from which to select the
 *                  character.
 *  \param inCode The integer ID of character in the master map.
 *  \return The string name of the character that \e inCode is mapped to.
 *  \sa glcGetMasterListc() 
 *  \sa glcGetMasterc()
 *  \sa glcGetMasteri()
 */
const GLCchar* glcGetMasterMap(GLint inMaster, GLint inCode)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar *s = NULL;
  GLCchar *buffer = NULL;
  GLint length = 0;
  FT_Face face = NULL;
  FT_UInt glyphIndex = 0;
  GLint i = 0;
  FT_ListNode node = NULL;

  /* Verify if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Check if inMaster and inCode are in legal bounds */
  if ((inMaster < 0) || (inCode < 0)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  for(node = state->masterList->head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    if (master->id == inMaster) break;
  }

  /* No master identified by inMaster has been found */
  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* We search for a font file that supports the requested inCode glyph */
  for (i = 0; (GLuint)i < __glcStrLstLen(master->faceFileName); i++) {
    /* Copy the Unicode string into a buffer
     * NOTE : we do not change the encoding format (or string type) of the file
     *        name since we suppose that the encoding format of the OS has not
     *        changed since the user provided the file name
     */
    s = __glcStrLstFindIndex(master->faceFileName, i);
    buffer = s->ptr;

    if (FT_New_Face(state->library, 
		    (const char*) buffer, 0, &face)) {
      /* Unable to load the face file, however this should not happen since
       * it has been succesfully loaded when the master was created.
       */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      face = NULL;
      continue;
    }

    /* Search, in the current font file, for a glyph that matches inCode */
    glyphIndex = FT_Get_Char_Index(face, inCode);
    if (glyphIndex)
      break; /* Found !!!*/

    /* No glyph in the font file matches inCode : close the font file and try
     * the next one
     */
    if (face) {
      FT_Done_Face(face);
      face = NULL;
    }
  }

  /* We have looked for the glyph in every font files of the master but did
   * not find a matching glyph => QUIT !!
   */
  if (!face)
    return GLC_NONE;
    
  /* The database gives the Unicode name in UCS1 encoding. We should now
   * change its encoding if needed.
   */
  s = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  if (!s) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  s->ptr = __glcNameFromCode(inCode);
  s->type = GLC_UCS1;
  if (!s->ptr) {
    __glcFree(s);
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Allocates memory to perform the conversion */
  length = __glcUniEstimate(s, state->stringType);
  buffer = __glcCtxQueryBuffer(state, length);
  if (!buffer) {
    __glcFree(s);
    /* __glcFree(content.dptr); */
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  /* Perform the conversion */
  __glcUniConvert(s, buffer, state->stringType, length);
  __glcFree(s);
  /* __glcFree(content.dptr); */
    
  return buffer;
}



/** \ingroup master
 *  This command returns a string attribute of the master identified by
 *  \e inMaster. The table below lists the string attributes that are
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
 *  \param inMaster The master for which an attribute value is needed.
 *  \param inAttrib The attribute for which the value is needed.
 *  \return The value that is associated with the attribute \e inAttrib.
 *  \sa glcGetMasteri()
 *  \sa glcGetMasterMap()
 *  \sa glcGetMasterListc()
 */
const GLCchar* glcGetMasterc(GLint inMaster, GLCenum inAttrib)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar *s = NULL;
  GLCchar *buffer = NULL;
  GLint length = 0;
  FT_ListNode node = NULL;

  /* Check parameter inAttrib */
  switch(inAttrib) {
  case GLC_FAMILY:
  case GLC_MASTER_FORMAT:
  case GLC_VENDOR:
  case GLC_VERSION:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if inMaster is in legal bounds */
  if (inMaster < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify if the thread owns a GLC context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  for(node = state->masterList->head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    if (master->id == inMaster) break;
  }

  /* No master identified by inMaster has been found */
  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
  
  /* Get the Unicode string which corresponds to the requested attribute */
  switch(inAttrib) {
  case GLC_FAMILY:
    s = master->family;
    break;
  case GLC_MASTER_FORMAT:
    s = master->masterFormat;
    break;
  case GLC_VENDOR:
    s = master->vendor;
    break;
  case GLC_VERSION:
    s = master->version;
    break;
  }

  /* Allocate memory in order to perform the conversion into the current
   * string type
   */
  length = __glcUniEstimate(s, state->stringType);
  buffer = __glcCtxQueryBuffer(state, length);
  if (buffer)
    /* Convert into the current string type */
    __glcUniConvert(s, buffer, state->stringType, length);
  else
    __glcRaiseError(GLC_RESOURCE_ERROR);

  return buffer;  
}



/** \ingroup master
 *  This command returns an integer attribute of the master identified by
 *  \e inMaster. The attribute is identified by \e inAttrib. The table below
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
 *  \param inMaster The master for which an attribute value is needed.
 *  \param inAttrib The attribute for which the value is needed.
 *  \return the value of the attribute \e inAttrib of the master identified
 *  by \e inMaster.
 *  \sa glcGetMasterc()
 *  \sa glcGetMasterMap()
 *  \sa glcGetMasterListc()
 */
GLint glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  FT_ListNode node = NULL;
  GLint count = 0;

  /* Check parameter inAttrib */
  switch(inAttrib) {
  case GLC_CHAR_COUNT:
  case GLC_FACE_COUNT:
  case GLC_IS_FIXED_PITCH:
  case GLC_MAX_MAPPED_CODE:
  case GLC_MIN_MAPPED_CODE:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if inMaster is in legal bounds */
  if (inMaster < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify that the thread owns a context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  for(node = state->masterList->head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    if (master->id == inMaster) break;
  }

  /* No master identified by inMaster has been found */
  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* returns the requested attribute */
  switch(inAttrib) {
  case GLC_CHAR_COUNT:
    for (node = master->charList->head, count = 0; node;
	 node = node->next, count++) {}
    return count;
  case GLC_FACE_COUNT:
    return __glcStrLstLen(master->faceList);
  case GLC_IS_FIXED_PITCH:
    return master->isFixedPitch;
  case GLC_MAX_MAPPED_CODE:
    return master->maxMappedCode;
  case GLC_MIN_MAPPED_CODE:
    return master->minMappedCode;
  }

  return 0;
}



/** \ingroup master
 *  This command appends the string \e inCatalog to the list
 *  \b GLC_CATALOG_LIST.
 *
 *  The catalog is represented as a zero-terminated string. The interpretation
 *  of this string is specified by the value that has been set using
 *  glcStringType().
 *
 *  A catalog is a path to a file named \c fonts.dir which is a list of
 *  masters. A master is a representation of a font that is stored outside
 *  QuesoGLC in a standard format such as TrueType or Type1.
 *
 *  A catalog defines the list of masters that can be instantiated (that is, be
 *  used as fonts) in a GLC context.
 *
 *  A font is a styllistically consistent set of glyphs that can be used to
 *  render some set of characters. Each font has a family name (for example
 *  Palatino) and a state variable that selects one of the faces (for example
 *  regular, bold, italic, bold italic) that the font contains. A typeface is
 *  the combination of a family and a face (for example Palatino Bold).
 *  \param inCatalog The catalog to append to the list \b GLC_CATALOG_LIST
 *  \sa glcGetList() with argument \b GLC_CATALOG_LIST
 *  \sa glcGeti() with argument \b GLC_CATALOG_COUNT
 *  \sa glcPrependCatalog()
 *  \sa glcRemoveCatalog()
 */
void glcAppendCatalog(const GLCchar* inCatalog)
{
  __glcContextState *state = NULL;

  /* Verify that the thread owns a context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* append the catalog */
  __glcCtxAddMasters(state, inCatalog, GL_TRUE);
}



/** \ingroup master
 *  This command prepends the string \e inCatalog to the list
 *  \b GLC_CATALOG_LIST
 *  \param inCatalog The catalog to prepend to the list \b GLC_CATALOG_LIST
 *  \sa glcAppendCatalog()
 *  \sa glcRemoveCatalog()
 */
void glcPrependCatalog(const GLCchar* inCatalog)
{
  __glcContextState *state = NULL;

  /* Verify that the thread owns a context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* prepend the catalog */
  __glcCtxAddMasters(state, inCatalog, GL_FALSE);
}



/** \ingroup master
 *  This command removes a string from the list \b GLC_CATALOG_LIST. It removes
 *  the string at offset \e inIndex from the first element in the list. The
 *  command raises \b GLC_PARAMETER_ERROR if \e inIndex is less than zero or is
 *  greater than or equal to the value of the variable \b GLC_CATALOG_COUNT.
 *  \param inIndex The string to remove from the catalog list
 *                 \b GLC_CATALOG_LIST
 *  \sa glcAppendCatalog()
 *  \sa glcPrependCatalog()
 */
void glcRemoveCatalog(GLint inIndex)
{
  __glcContextState *state = NULL;
  GLint count = 0;

  /* Verify that the thread owns a context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Verify that the parameter inIndex is in legal bounds */
  __glcLock();
  count = __glcStrLstLen(__glcCommonArea->catalogList);
  __glcUnlock();
  if ((inIndex < 0) || ((GLuint)inIndex >= count)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* removes the catalog */
  __glcCtxRemoveMasters(state, inIndex);
}
