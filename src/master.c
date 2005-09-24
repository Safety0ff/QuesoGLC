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
 * defines the so-called "Master commands" described in chapter 3.6 of the GLC
 * specs.
 */

/** \defgroup master Master Commands
 *  Commands to create, manage and destroy masters.
 *
 *  A master is a representation of a font that is stored outside QuesoGLC in a
 *  standard format such as TrueType or Type1.
 *
 *  QuesoGLC maps the font files into master objects that are visible through
 *  the GLC API. A group of font files from a single typeface family will be
 *  mapped into a single GLC master object that has multiple faces. For
 *  example, the files \c Courier.pfa, \c Courier-Bold.pfa,
 *  \c Courier-BoldOblique.pfa, and \c Courier-Oblique.pfa are visible through
 *  the GLC API as a single master with \c GLC_VENDOR="Adobe",
 *  \c GLC_FAMILY="Courier", \c GLC_MASTER_FORMAT="Type1", \c GLC_FACE_COUNT=4
 *  and \c GLC_FACE_LIST=("Regular", "Bold", "Bold Oblique", "Oblique")
 *
 *  Some GLC commands have a parameter \e inMaster. This parameter is an offset
 *  from the the first element in the GLC master list. The command raises
 *  \b GLC_PARAMETER_ERROR if \e inMaster is less than zero or is greater than
 *  or equal to the value of the variable \b GLC_MASTER_COUNT.
 */

#include "internal.h"
#include FT_LIST_H



/* Most master commands need to check that :
 *   1. The current thread owns a context state
 *   2. The master identifier 'inMaster' is legal
 * This internal function does both checks and returns the pointer to the
 * __glcMaster object that is identified by 'inMaster'.
 */
__glcMaster* __glcVerifyMasterParameters(GLint inMaster)
{
  __glcContextState *state = __glcGetCurrent();
  FT_ListNode node = NULL;
  __glcMaster *master = NULL;

  /* Check if the current thread owns a context state */
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify if the master identifier is in legal bounds */
  for (node = state->masterList.head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    if (master->id == inMaster) break;
  }

  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Returns the __glcMaster object identified by inMaster */
  return master;
}



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
  __glcContextState *state = __glcGetCurrent();
  __glcMaster *master = NULL;
  FT_ListNode node = NULL;
  FcChar32 base, next, map[FC_CHARSET_MAP_SIZE];
  FcChar32 count = 0;
  int i = 0;
  int j = 0;

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

  /* Verify if inIndex is in legal bounds */
  if (inIndex < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify that the thread has a current context and that the master identified
   * by 'inMaster' exists.
   */
  master = __glcVerifyMasterParameters(inMaster);
  if (!master)
    return GLC_NONE;

  switch(inAttrib) {
  case GLC_CHAR_LIST:
    base = FcCharSetFirstPage(master->charList, map, &next);
    do {
      for (i = 0; i < FC_CHARSET_MAP_SIZE; i++) {
        FcChar32 value = __glcCharSetPopCount(map[i]);
        if (count + value >= inIndex + 1) {
          for (j = 0; j < 32; j++) {
            if ((map[i] >> j) & 1) count++;
            if (count == inIndex + 1)
              return glcGetMasterMap(inMaster, base + (i << 5) + j);
          }
        }
        count += value;  
      }
      base = FcCharSetNextPage(master->charList, map, &next);
    } while (base != FC_CHARSET_DONE);
    /* The character has not been found */
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;

  case GLC_FACE_LIST:
    /* Get the face name */
    for (node = master->faceList.head; inIndex && node; node = node->next,
	 inIndex--);
    if (!node) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    return __glcConvertFromUtf8ToBuffer(state,
				 ((__glcFaceDescriptor*)node)->styleName,
					state->stringType);
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
}



/** \ingroup master
 *  This command returns the string name of the character that the master
 *  identified by \e inMaster maps \e inCode to.
 *
 *  Every master has associated with it a master map, which is a table of
 *  entries that map integer values to the name string that identifies the
 *  character.
 *
 *  Every character code used in QuesoGLC is an element of the Unicode
 *  Character Database (UCD) defined by the standards ISO/IEC 10646:2003 and
 *  Unicode 4.0.1 (unless otherwise specified). A Unicode code point is denoted
 *  as \e U+hexcode, where \e hexcode is a sequence of hexadecimal digits. Each
 *  Unicode code point corresponds to a character that has a unique name
 *  string. For example, the code \e U+41 corresponds to the character
 *  <em>LATIN CAPITAL LETTER A</em>.
 *
 *  If the master does not map \e inCode, the command returns \b GLC_NONE.
 *  \note While you cannot change the map of a master, you can change the map
 *  of a font using glcFontMap().
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
  __glcContextState *state = __glcGetCurrent();
  __glcMaster *master = NULL;
  GLCchar *buffer = NULL;
  FcChar8* name = NULL;
  GLint code = 0;

  /* Check if inCode is in legal bounds */
  if (inCode < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify that the thread has a current context and that the master identified
   * by 'inMaster' exists.
   */
  master = __glcVerifyMasterParameters(inMaster);
  if (!master)
    return GLC_NONE;

  /* Get the character code converted to the UCS-4 format.
   * See comments at the definition of __glcConvertGLintToUcs4() for known
   * limitations
   */
  code = __glcConvertGLintToUcs4(state, inCode);
  if (code < 0)
    return NULL;

  if (!FcCharSetHasChar(master->charList, (FcChar32)code))
    return GLC_NONE;
    
  /* The database gives the Unicode name in UCS1 encoding. We should now
   * change its encoding if needed.
   */
  name = __glcNameFromCode(code);
  if (!name) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Performs the conversion */
  buffer = __glcConvertFromUtf8ToBuffer(state, name, state->stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

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
  __glcContextState *state = __glcGetCurrent();
  __glcMaster *master = NULL;
  GLCchar *buffer = NULL;
  FcChar8* s = NULL;

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

  /* Verify that the thread has a current context and that the master identified
   * by 'inMaster' exists.
   */
  master = __glcVerifyMasterParameters(inMaster);
  if (!master)
    return GLC_NONE;

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

  /* Convert the string and store it in the context buffer */
  buffer = __glcConvertFromUtf8ToBuffer(state, s, state->stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

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
 *  \n If the requested master attribute is \b GLC_IS_FIXED_PITCH then the
 *  command returns \b GL_TRUE if and only if each face of the master identified
 *  by \e inMaster has a fixed pitch.
 *  \param inMaster The master for which an attribute value is needed.
 *  \param inAttrib The attribute for which the value is needed.
 *  \return The value of the attribute \e inAttrib of the master identified
 *  by \e inMaster.
 *  \sa glcGetMasterc()
 *  \sa glcGetMasterMap()
 *  \sa glcGetMasterListc()
 */
GLint glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
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

  /* Verify that the thread has a current context and that the master identified
   * by 'inMaster' exists.
   */
  master = __glcVerifyMasterParameters(inMaster);
  if (!master)
    return GLC_NONE;

  /* returns the requested attribute */
  switch(inAttrib) {
  case GLC_CHAR_COUNT:
    return FcCharSetCount(master->charList);
  case GLC_FACE_COUNT:
    for (node = master->faceList.head; node; node = node->next, count++);
    return count;
  case GLC_IS_FIXED_PITCH:
    for (node = master->faceList.head; node; node = node->next) {
      __glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)node;
      if (!faceDesc->isFixedPitch)
        return GL_FALSE;
    }
    return GL_TRUE;
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
 *  A catalog is a path to a list of masters. A master is a representation of a
 *  font that is stored outside QuesoGLC in a standard format such as TrueType
 *  or Type1.
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
 *
 *  QuesoGLC also destroys the masters that are defined in the corresponding
 *  catalog.
 *  \param inIndex The string to remove from the catalog list
 *                 \b GLC_CATALOG_LIST
 *  \sa glcAppendCatalog()
 *  \sa glcPrependCatalog()
 */
void glcRemoveCatalog(GLint inIndex)
{
  __glcContextState *state = NULL;

  /* Verify that the thread owns a context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Verify that the parameter inIndex is in legal bounds */
  if (inIndex < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* removes the catalog */
  __glcCtxRemoveMasters(state, inIndex);
}
