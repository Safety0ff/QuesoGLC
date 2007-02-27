/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
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
 *  Every master has an associated character map. A character map is a table of
 *  entries that maps integer values to the name string that identifies the
 *  characters. Unlike fonts character maps, the character map of a master can
 *  not be modified.
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

#ifndef __WIN32__ 	 
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "internal.h"
#include FT_LIST_H



/* Most master commands need to check that :
 *   1. The current thread owns a context state
 *   2. The master identifier 'inMaster' is legal
 * This internal function does both checks and returns the pointer to the
 * __glcMaster object that is identified by 'inMaster'.
 */
FcPattern* __glcVerifyMasterParameters(GLint inMaster)
{
  __GLCcontext *ctx = __glcGetCurrent();

  /* Check if the current thread owns a context state */
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify if the master identifier is in legal bounds */
  if (inMaster >= GLC_ARRAY_LENGTH(ctx->masterHashTable)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  return __glcGetPatternFromMasterID(inMaster, ctx);
}



/* Since masters do not actually exist (they are an abstraction of the
 * Fontconfig data), their charmaps are not stored in memory. So when we need
 * the charmap of a master, we have to retrieve it from Fontconfig.
 */
static __GLCcharMap* __glcGetMasterCharMap(FcFontSet* inFontSet)
{
  int i = 0;
  FcResult result = FcResultMatch;
  __GLCcharMap* charMap = NULL; 

  charMap = __glcCharMapCreate(NULL);
  if (!charMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (i = 0; i < inFontSet->nfont; i++) {
    FcCharSet* charSet = NULL;
    __GLCcharMap* charList = NULL;

    result = FcPatternGetCharSet(inFontSet->fonts[i], FC_CHARSET, 0, &charSet);
    assert(result != FcResultTypeMismatch);
    charList = __glcCharMapCreate(charSet);
    if (!charList) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcCharMapDestroy(charMap);
      return NULL;
    }
    if (!__glcCharMapUnion(charMap, charList)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcCharMapDestroy(charMap);
      __glcCharMapDestroy(charList);
	return NULL;
      }
    __glcCharMapDestroy(charList);
  }

  return charMap;
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
const GLCchar* APIENTRY glcGetMasterListc(GLint inMaster, GLCenum inAttrib,
				 GLint inIndex)
{
  __GLCcontext *ctx = __glcGetCurrent();
  FcPattern *pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  __GLCcharMap* charMap = NULL;
  FcResult result = FcResultMatch;
  FcChar8* string = NULL;
  GLCchar* element = NULL;

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

  /* Verify that the thread has a current context and that the master
   * identified by 'inMaster' exists.
   */
  pattern = __glcVerifyMasterParameters(inMaster);
  if (!pattern)
    return GLC_NONE;

  objectSet = FcObjectSetBuild(FC_STYLE, FC_CHARSET, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GLC_NONE;
  }
  fontSet = FcFontList(ctx->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GLC_NONE;
  }

  /* return the requested attribute */
  switch(inAttrib) {
  case GLC_CHAR_LIST:
    charMap = __glcGetMasterCharMap(fontSet);
    if (!charMap) {
      FcFontSetDestroy(fontSet);
      FcPatternDestroy(pattern);
      return GLC_NONE;
    }
    string = __glcCharMapGetCharNameByIndex(charMap, inIndex, ctx);
    if (!string) {
      FcFontSetDestroy(fontSet);
      FcPatternDestroy(pattern);
      __glcCharMapDestroy(charMap);
      return GLC_NONE;
    }
    break;
  case GLC_FACE_LIST:
    /* Get the face name */
    if (inIndex >= fontSet->nfont) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      FcFontSetDestroy(fontSet);
      FcPatternDestroy(pattern);
      return GLC_NONE;
    }
    result = FcPatternGetString(fontSet->fonts[inIndex], FC_STYLE, 0, &string);
    assert(result != FcResultTypeMismatch);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }


  /* Convert it from UTF-8 to the current string type and return */
  element = __glcConvertFromUtf8ToBuffer(ctx, string,
					 ctx->stringState.stringType);
  FcFontSetDestroy(fontSet);
  FcPatternDestroy(pattern);
  if (charMap)
    __glcCharMapDestroy(charMap);
  return element;
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
const GLCchar* APIENTRY glcGetMasterMap(GLint inMaster, GLint inCode)
{
  FcPattern *pattern = __glcVerifyMasterParameters(inMaster);
  __GLCcontext *ctx = __glcGetCurrent();
  GLint code = 0;

  if (pattern) {
    FcObjectSet* objectSet = NULL;
    FcFontSet *fontSet = NULL;
    __GLCcharMap* charMap = NULL;
    GLCchar* result = NULL;

    objectSet = FcObjectSetBuild(FC_STYLE, FC_CHARSET, NULL);
    if (!objectSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcPatternDestroy(pattern);
      return GLC_NONE;
    }
    fontSet = FcFontList(ctx->config, pattern, objectSet);
    FcObjectSetDestroy(objectSet);
    if (!fontSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcPatternDestroy(pattern);
      return GLC_NONE;
    }

    charMap = __glcGetMasterCharMap(fontSet);
    if (!charMap) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcPatternDestroy(pattern);
      FcFontSetDestroy(fontSet);
      return GLC_NONE;
    }

    /* Get the character code converted to the UCS-4 format */
    code = __glcConvertGLintToUcs4(ctx, inCode);
    if (code < 0)
      return GLC_NONE;

    result = __glcCharMapGetCharName(charMap, code, ctx);
    FcPatternDestroy(pattern);
    FcFontSetDestroy(fontSet);
    __glcCharMapDestroy(charMap);
    return result;
  }
  else
    return GLC_NONE;
}



/* This subroutine is called whenever the user wants to access to informations
 * that have not been loaded from the font files yet. In order to reduce disk
 * accesses, informations such as the master format, full name or version are
 * read "just in time" i.e. only when the user requests them.
 */
static FcChar8* __glcGetMasterInfoJustInTime(FcPattern* inPattern,
					     __GLCcontext* inContext,
					     GLCenum inAttrib)
{
  FcChar8* result = NULL;
  __GLCfaceDescriptor* faceDesc = __glcGetFaceDescFromPattern(inPattern,
							      inContext);

  if (!faceDesc) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  result = __glcFaceDescGetFontFormat(faceDesc, inContext, inAttrib);
  __glcFaceDescDestroy(faceDesc, inContext);
  return result;
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
 *    <tr>
 *      <td><b>GLC_FULL_NAME_SGI</b></td> <td>0x8002</td>
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
const GLCchar* APIENTRY glcGetMasterc(GLint inMaster, GLCenum inAttrib)
{
  __GLCcontext *ctx = __glcGetCurrent();
  GLCchar *buffer = NULL;
  FcChar8* s = NULL;
  FcPattern* pattern = NULL;
  FcResult result = FcResultMatch;

  /* Check parameter inAttrib */
  switch(inAttrib) {
  case GLC_FAMILY:
  case GLC_MASTER_FORMAT:
  case GLC_VENDOR:
  case GLC_VERSION:
  case GLC_FULL_NAME_SGI:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Verify that the thread has a current context and that the master
   * identified by 'inMaster' exists.
   */
  pattern = __glcVerifyMasterParameters(inMaster);
  if (!pattern)
    return GLC_NONE;

  /* Get the Unicode string which corresponds to the requested attribute */
  switch(inAttrib) {
  case GLC_FAMILY:
    result = FcPatternGetString(pattern, FC_FAMILY, 0, &s);
    assert(result != FcResultTypeMismatch);
    break;
  case GLC_VENDOR:
    result = FcPatternGetString(pattern, FC_FOUNDRY, 0, &s);
    assert(result != FcResultTypeMismatch);
    break;
  case GLC_VERSION:
  case GLC_FULL_NAME_SGI:
  case GLC_MASTER_FORMAT:
    s = __glcGetMasterInfoJustInTime(pattern, ctx, inAttrib);
    break;
  }

  if (!s) {
    FcPatternDestroy(pattern);
    return GLC_NONE;
  }

  /* Convert the string and store it in the context buffer */
  buffer = __glcConvertFromUtf8ToBuffer(ctx, s,
					ctx->stringState.stringType);
  FcPatternDestroy(pattern);

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
 *  command returns \b GL_TRUE if and only if each face of the master
 *  identified by \e inMaster has a fixed pitch.
 *  \param inMaster The master for which an attribute value is needed.
 *  \param inAttrib The attribute for which the value is needed.
 *  \return The value of the attribute \e inAttrib of the master identified
 *  by \e inMaster.
 *  \sa glcGetMasterc()
 *  \sa glcGetMasterMap()
 *  \sa glcGetMasterListc()
 */
GLint APIENTRY glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
  GLint count = 0;
  FcPattern *pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  __GLCcharMap* charMap = NULL;
  __GLCcontext* ctx = NULL;

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

  /* Verify that the thread has a current context and that the master
   * identified by 'inMaster' exists.
   */
  ctx = __glcGetCurrent();
  pattern = __glcVerifyMasterParameters(inMaster);
  if (!pattern)
    return GLC_NONE;

  if (inAttrib == GLC_IS_FIXED_PITCH) {
    /* Is this a fixed font ? */
    FcResult result = FcResultMatch;
    int fixed = 0;

    result = FcPatternGetInteger(pattern, FC_SPACING, 0, &fixed);
    assert(result != FcResultTypeMismatch);
    FcPatternDestroy(pattern);
    return fixed ? GL_TRUE: GL_FALSE;
  }

  objectSet = FcObjectSetBuild(FC_STYLE, FC_CHARSET, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GLC_NONE;
  }
  fontSet = FcFontList(ctx->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GLC_NONE;
  }

  if (inAttrib != GLC_FACE_COUNT) {
    charMap = __glcGetMasterCharMap(fontSet);
    if (!charMap) {
      FcFontSetDestroy(fontSet);
      FcPatternDestroy(pattern);
      return GLC_NONE;
    }
  }

  /* return the requested attribute */
  switch(inAttrib) {
  case GLC_CHAR_COUNT:
    count = __glcCharMapGetCount(charMap);
    break;
  case GLC_FACE_COUNT:
    count = fontSet->nfont;
    break;
  case GLC_MAX_MAPPED_CODE:
    count = __glcCharMapGetMaxMappedCode(charMap);
    break;
  case GLC_MIN_MAPPED_CODE:
    count = __glcCharMapGetMinMappedCode(charMap);
    break;
  }

  if (charMap)
    __glcCharMapDestroy(charMap);
  FcFontSetDestroy(fontSet);
  FcPatternDestroy(pattern);
  return count;
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
void APIENTRY glcAppendCatalog(const GLCchar* inCatalog)
{
  __GLCcontext *ctx = NULL;
  struct stat dirStat;

  /* If inCatalog is NULL then there is no point in continuing */
  if (!inCatalog)
    return;

  /* Check that 'inCatalog' points to a directory that can be read */ 	 
  #ifdef __WIN32__ 	 
  if (_access((const char*)inCatalog, 0)) { 	 
  #else 	 
  if (access((char *)inCatalog, R_OK) < 0) { 	 
  #endif 	 
    /* May be something more explicit should be done */ 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  }
  /* Check that 'inCatalog' is a directory */ 	 
  if (stat((char *)inCatalog, &dirStat) < 0) { 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  } 	 
  #ifdef __WIN32__ 	 
  if (!(dirStat.st_mode & _S_IFDIR)) { 	 
  #else 	 
  if (!S_ISDIR(dirStat.st_mode)) { 	 
  #endif 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  } 	 

  /* Verify that the thread owns a context */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* append the catalog */
  if (!FcStrSetAdd(ctx->catalogList, inCatalog)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  if (!FcConfigAppFontAddDir(ctx->config, (const unsigned char*)inCatalog)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  __glcUpdateHashTable(ctx);
}



/** \ingroup master
 *  This command prepends the string \e inCatalog to the list
 *  \b GLC_CATALOG_LIST
 *  \param inCatalog The catalog to prepend to the list \b GLC_CATALOG_LIST
 *  \sa glcAppendCatalog()
 *  \sa glcRemoveCatalog()
 */
void APIENTRY glcPrependCatalog(const GLCchar* inCatalog)
{
  __GLCcontext *ctx = NULL;
  FcStrSet* newCatalog = NULL;
  FcStrList* iterator = NULL;
  FcChar8* catalog = NULL;
  struct stat dirStat;

  /* If inCatalog is NULL then there is no point in continuing */
  if (!inCatalog)
    return;

  /* Check that 'inCatalog' points to a directory that can be read */ 	 
  #ifdef __WIN32__ 	 
  if (_access((const char*)inCatalog, 0)) { 	 
  #else 	 
  if (access((char *)inCatalog, R_OK) < 0) { 	 
  #endif 	 
    /* May be something more explicit should be done */ 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  }
  /* Check that 'inCatalog' is a directory */ 	 
  if (stat((char *)inCatalog, &dirStat) < 0) { 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  } 	 
  #ifdef __WIN32__ 	 
  if (!(dirStat.st_mode & _S_IFDIR)) { 	 
  #else 	 
  if (!S_ISDIR(dirStat.st_mode)) { 	 
  #endif 	 
    __glcRaiseError(GLC_PARAMETER_ERROR); 	 
	return; 	 
  } 	 

  /* Verify that the thread owns a context */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* prepend the catalog */
  newCatalog = FcStrSetCreate();
  if (!newCatalog) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  if (!FcStrSetAdd(newCatalog, inCatalog)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcStrSetDestroy(newCatalog);
    return;
  }
  iterator = FcStrListCreate(ctx->catalogList);
  if (!iterator) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcStrSetDestroy(newCatalog);
    return;
  }
  for (catalog = FcStrListNext(iterator); catalog;
       catalog = FcStrListNext(iterator)) {
    if (!FcStrSetAdd(newCatalog, catalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcStrSetDestroy(newCatalog);
      FcStrListDone(iterator);
      return;
    }
  }
  FcStrListDone(iterator);
  FcStrSetDestroy(ctx->catalogList);
  ctx->catalogList = newCatalog;

  if (!FcConfigAppFontAddDir(ctx->config, (const unsigned char*)inCatalog)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcStrSetDel(ctx->catalogList, inCatalog);
    return;
  }
  __glcUpdateHashTable(ctx);
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
void APIENTRY glcRemoveCatalog(GLint inIndex)
{
  __GLCcontext *ctx = NULL;
  FcChar8* catalog = NULL;
  FcStrList* iterator = NULL;
  FcStrSet* newCatalog = NULL;
  FT_ListNode node = NULL;

  /* Verify that the thread owns a context */
  ctx = __glcGetCurrent();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Verify that the parameter inIndex is in legal bounds */
  if (inIndex < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* removes the catalog */
  iterator = FcStrListCreate(ctx->catalogList);
  if (!iterator) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  for (catalog = FcStrListNext(iterator); catalog && inIndex;
       catalog = FcStrListNext(iterator), inIndex);
  FcStrListDone(iterator);

  if (inIndex || !catalog) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  newCatalog = FcStrSetCreate();
  if (!newCatalog) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  FcStrSetDel(ctx->catalogList, catalog);
  FcConfigAppFontClear(ctx->config);
  iterator = FcStrListCreate(ctx->catalogList);
  if (!iterator) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcStrSetDestroy(newCatalog);
    return;
  }
  for (catalog = FcStrListNext(iterator); catalog;
       catalog = FcStrListNext(iterator)) {
    if (!FcStrSetAdd(newCatalog, catalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcStrSetDestroy(ctx->catalogList);
      ctx->catalogList = newCatalog;
      FcStrListDone(iterator);
      return;
    }
    if (!FcConfigAppFontAddDir(ctx->config, catalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcStrSetDel(newCatalog, catalog);
      FcStrSetDestroy(ctx->catalogList);
      ctx->catalogList = newCatalog;
      FcStrListDone(iterator);
      return;
    }
  }
  FcStrListDone(iterator);
  __glcCreateHashTable(ctx);

  for (node = ctx->fontList.head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)(node->data);
    FcPattern* pattern = __glcGetPatternFromMasterID(font->parentMasterID, ctx);
    FcChar32 hashValue = 0;
    FcChar32* hashTable = (FcChar32*)GLC_ARRAY_DATA(ctx->masterHashTable);
    int length = GLC_ARRAY_LENGTH(ctx->masterHashTable);
    int i = 0;

    if (!pattern) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
    hashValue = FcPatternHash(pattern);
    for (i = 0; i < length; i++) {
      if (hashValue == hashTable[i])
	break;
    }

    if (i == length)
      glcDeleteFont(font->id);
  }
}
