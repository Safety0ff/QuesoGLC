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

/* This file defines the so-called "Master commands" described in chapter 3.6
 * of the GLC specs
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_LIST_H

/* glcGetMasterListc:
 *   This command returns a string from a string list that is an attribute of
 *   the master identified by inMaster. The string list is identified by
 *   inAttrib. The command returns the string at offset inIndex from the first
 *   element in this string list. The command raises GLC_PARAMETER_ERROR if
 *   inIndex is less than zero or is greater than or equal to the value of the
 *   list element count attribute.
 */
const GLCchar* glcGetMasterListc(GLint inMaster, GLCenum inAttrib, GLint inIndex)
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

  /* Verify if a context is current to the thread */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Verify if inMaster is in legal bounds */
  if (inMaster < 0) {
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
    
  switch(inAttrib) {
  case GLC_CHAR_LIST:
    /* Verify if inIndex is in legal bounds */
    if ((inIndex < 0) || (inIndex >= master->charListCount)) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    else {
      FT_ListNode node = master->charList->head;
      GLint i;

      for (i = 0; i < inIndex; i++)
	node = node->next;

      return glcGetMasterMap(inMaster, *((FT_ULong*)node->data));
    }
  case GLC_FACE_LIST:
    /* Verify if inIndex is in legal bounds */
    if (inIndex < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    else {
      /* Get the face name */
      s = __glcStrLstFindIndex(master->faceList, inIndex);
      if (!s) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
      }
      break;
    }
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

/* glcGetMasterMap:
 *   This command returns the string name of the character that the master
 *   identified by inMaster maps inCode to. If the master does not map inCode,
 *   the command returns GLC_NONE.
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
  datum key, content;
  GLint i = 0;
  FT_ListNode node = NULL;

  /* Verify if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
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
    buffer = __glcCtxQueryBuffer(state, __glcUniLenBytes(s));
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    __glcUniDup(s, buffer, __glcUniLenBytes(s));

    if (FT_New_Face(state->library, 
		    (const char*) buffer, 0, &face)) {
      /* Unable to load the face file, however this should not happen since
	 it has been succesfully loaded when the master was created */
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
  if ((GLuint)i == __glcStrLstLen(master->faceFileName))
    if (face) {
      FT_Done_Face(face);
      face = NULL;
      return GLC_NONE;
    }
    
  /* We have found a font file that contains the glyph that the user is looking
   * for. We now have to find its Unicode name.
   */
  key.dsize = sizeof(GLint);
  key.dptr = (char *)&inCode;
  /* Search for the Unicode into the database */
  content = __glcDBFetch(__glcCommonArea->unidb1, key);
  if (!content.dptr) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE; /* Not found !! */
  }

  /* The database gives the Unicode name in UCS1 encoding. We should now
   * change its encoding if needed.
   */
  s = (__glcUniChar*)__glcMalloc(sizeof(__glcUniChar));
  s->ptr = content.dptr;
  s->type = GLC_UCS1;

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

/* glcGetMasterc:
 *   This command returns a string attribute of the master identified by
 *   inMaster.
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

  /* Verify if the thread owns a GLC context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
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

/* glcGetMasteri:
 *   This command returns an integer attribute of the master identified by
 *   inMaster. The attribute is identified by inAttrib.
 */
GLint glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  FT_ListNode node = NULL;

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

  /* Verify that the thread owns a context */
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

  /* No master identified by inMaster has been found */
  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* returns the requested attribute */
  switch(inAttrib) {
  case GLC_CHAR_COUNT:
    return master->charListCount;
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

/* glcAppendCatalog:
 *   This command appends the string inCatalog to the list GLC_CATALOG_LIST
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

/* glcPrependCatalog:
 *   This command prepends the string inCatalog to the list GLC_CATALOG_LIST
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

/* glcRemoveCatalog:
 *   This command removes a string from the list GLC_CATALOG_LIST. It removes
 *   the string at offset inIndex from the first element in the list. The
 *   command raises GLC_PARAMETER_ERROR if inIndex is less than zero or is
 *   greater than or equal to the value of the variable GLC_CATALOG_COUNT.
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
  if ((inIndex < 0)
      || ((GLuint)inIndex >= __glcStrLstLen(state->catalogList))) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* removes the catalog */
  __glcCtxRemoveMasters(state, inIndex);
}
