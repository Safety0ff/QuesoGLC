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
 * defines the so-called "Context commands" described in chapter 3.5 of the GLC
 * specs.
 */

/** \defgroup context Context Commands
 *  Commands to get or modify informations of the context state of the current
 *  thread.
 */

#include <stdlib.h>
#include <string.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_LIST_H



/** \ingroup context
 *  This command assigns the value \e inFunc to the callback function variable
 *  identified by \e inOpCode which must be chosen in the following table.
 *  <center>
 *  <table>
 *  <caption>Callback function variables</caption>
 *    <tr>
 *      <td>Name</td>
 *      <td>Enumerant</td>
 *      <td>Initial value</td>
 *      <td>Type signature</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_OP_glcUnmappedCode</b></td>
 *      <td>0x0020</td>
 *      <td><b>GLC_NONE</b></td>
 *      <td><b>GLboolean (*)(GLint)</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *
 *  The callback function can access can access to client data in a thread-safe
 *  manner with glcGetPointer().
 *  \param inOpcode Type of the callback function
 *  \param inFunc Callback function
 *  \sa glcGetCallbackFunc()
 *  \sa glcGetPointer()
 *  \sa glcDataPointer()
 */
void glcCallbackFunc(GLCenum inOpcode, GLCfunc inFunc)
{
  __glcContextState *state = NULL;

  /* Check parameters */
  if (inOpcode != GLC_OP_glcUnmappedCode) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  state->callback = inFunc;
}



/** \ingroup context
 *  This command assigns the value \e inPointer to the variable
 *  \b GLC_DATA_POINTER. It is used for access to client data from a callback
 *  function.
 *
 *  \e glcDataPointer provides a way to store in a GLC context a pointer to
 *  arbitrary application data. A GLC callback fucntion can subsequently use
 *  the command glcGetPointer() to obtain access to these data in a thread-safe
 *  manner.
 *  \param inPointer The pointer to assign to \b GLC_DATA_POINTER
 *  \sa glcGetPointer()
 *  \sa glcCallbackFunc()
 */
void glcDataPointer(GLvoid *inPointer)
{
  __glcContextState *state = NULL;

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  state->dataPointer = inPointer;
}



/* This functions destroys the display lists and the texture objects that
 * are associated with a context identified by inState. The corresponding
 * linked lists are also deleted.
 */
void __glcDeleteGLObjects(__glcContextState *inState)
{
  FT_ListNode node = NULL;
  __glcMaster* master = NULL;

  /* Delete display lists and texture objects */
  for(node = inState->masterList.head; node; node = node->next) {
    master = (__glcMaster*)node->data;
    FT_List_Finalize(&master->displayList, NULL,
                     &__glcCommonArea.memoryManager, NULL);
    FT_List_Finalize(&master->textureObjectList, __glcTextureObjectDestructor,
		     &__glcCommonArea.memoryManager, NULL);
  }
}



/** \ingroup context
 *  This command causes GLC to issue a sequence of GL commands to delete all
 *  of the GL objects that it owns.
 *
 *  GLC uses the command \c glDeleteLists to delete all of the GL objects named
 *  in \b GLC_LIST_OBJECT_LIST and uses the command \c glDeleteTextures to
 *  delete all of the GL objects named in \b GLC_TEXTURE_OBJECT_LIST. When an
 *  execution of glcDeleteGLObjects finishes, both of these lists are empty.
 *  \note \e glcDeleteGLObjects deletes only the objects that the current
 *        context owns, not all objects in all contexts.
 *  \sa glcGetListi()
 */
void glcDeleteGLObjects(void)
{
  __glcContextState *state = NULL;

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  __glcDeleteGLObjects(state);
}



/* This internal function is used by both glcEnable/glcDisable since they
 * actually do the same job : put a value into a member of the
 * __glcContextState struct. The only difference is the value that it puts.
 */
static void __glcDisable(GLCenum inAttrib, GLboolean value)
{
  __glcContextState *state = NULL;

  /* Check the parameters. 
   * NOTE : we do not need to check 'value' since it has been generated
   * internally.
   */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
  case GLC_GL_OBJECTS:
  case GLC_MIPMAP:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Assigns the value to the member identified by inAttrib */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
    state->autoFont = value;
    break;
  case GLC_GL_OBJECTS:
    state->glObjects = value;
    break;
  case GLC_MIPMAP:
    state->mipmap = value;
  }
}



/** \ingroup context
 *  This command assigns the value \b GL_FALSE to the boolean variable
 *  identified by \e inAttrib.
 *  <center>
 *  <table>
 *  <caption>Boolean variables</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td> <td>Initial value</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_AUTO_FONT</b></td> <td>0x0010</td> <td><b>GL_TRUE</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_GL_OBJECTS</b></td> <td>0x0011</td> <td><b>GL_TRUE</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MIPMAP</b></td> <td>0x0012</td> <td><b>GL_TRUE</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inAttrib A symbolic constant indicating a GLC capability.
 *  \sa glcIsEnabled()
 *  \sa glcEnable()
 */
void glcDisable(GLCenum inAttrib)
{
  __glcDisable(inAttrib, GL_FALSE);
}



/** \ingroup context
 *  This command assigns the value \b GL_TRUE to the boolean variable
 *  identified by \e inAttrib which must be chosen in the table above.
 *
 *  - \b GLC_AUTO_FONT : if enabled, GLC provides a font when the current GLC
 *    context has no fonts (i.e. provides a font to an application if the
 *    application does not want to set up fonts itself.
 *  - \b GLC_GL_OBJECTS : if enabled, GLC stores fonts in GL lists and textures
 *    in GL texture objects.
 *  - \b GLC_MIPMAP : if enabled, texture objects used by GLC are mipmapped
 *    using GLU's \c gluBuild2DMipmaps.
 *
 *  \param inAttrib A symbolic constant indicating a GLC capability.
 *  \sa glcDisable()
 *  \sa glcIsEnabled()
 */
void glcEnable(GLCenum inAttrib)
{
  __glcDisable(inAttrib, GL_TRUE);
}



/** \ingroup context
 *  This command returns the value of the callback function variable
 *  identified by \e inOpcode. Currently, \e inOpcode can only have the
 *  value \b GLC_OP_glcUnmappedCode. Its initial value and the type
 *  signature are defined in the table shown in glcCallbackFunc()'s
 *  definition.
 *  \param inOpcode The callback function to be retrieved
 *  \return the value of the callback function variable
 *  \sa glcCallbackFunc()
 */
GLCfunc glcGetCallbackFunc(GLCenum inOpcode)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  if (inOpcode != GLC_OP_glcUnmappedCode) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  return state->callback;
}



/** \ingroup context
 *  This command returns the string at offset \e inIndex from the first element
 *  in the string list identified by \e inAttrib which must be chosen in the
 *  table below :
 *  <center>
 *  <table>
 *  <caption>String lists</caption>
 *    <tr>
 *      <td>Name</td>
 *      <td>Enumerant</td>
 *      <td>Initial value</td>
 *      <td>Element count variable</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_CATALOG_LIST</b></td>
 *      <td>0x0080</td>
 *      <td>\<empty list\></td>
 *      <td><b>GLC_CATALOG_COUNT</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *
 *  The command raises a \b GLC_PARAMETER_ERROR if \e inIndex is less than zero
 *  or is greater than or equal to the value of the list's element count
 *  variable.
 *  \param inAttrib The string list attribute
 *  \param inIndex The index from which to retrieve an element.
 *  \return the string list element
 *  \sa glcGetListi()
 */
const GLCchar* glcGetListc(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  FcStrList* iterator = NULL;
  FcChar8* catalog = NULL;
  GLCchar* buffer = NULL;

  /* Check the parameters */
  if (inAttrib != GLC_CATALOG_LIST) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* NOTE : at this stage we can not verify if inIndex is greater than or equal
   * to the last element index. In order to perform such a verification we
   * would need to have the current context states but GLC specs says that we
   * should first check the parameters _then_ the current context (section 2.2
   * of specs). We are done !
   */
  if (inIndex < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Get the string at offset inIndex */
  __glcLock();
  iterator = FcStrListCreate(__glcCommonArea.catalogList);
  if (!iterator) {
    __glcUnlock();
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }
  for (catalog = FcStrListNext(iterator); catalog && inIndex;
	catalog = FcStrListNext(iterator), inIndex--);
  FcStrListDone(iterator);
  __glcUnlock();

  /* Now we can check if inIndex identifies a member of the string set or 
   * not */
  if (!catalog) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Two remarks have to be made concerning the following code :
   * 1. We do not return a pointer that points to the actual location of the
   *    string in order to prevent the user to modify it. Instead QuesoGLC
   *    returns a pointer that points to a copy of the requested data.
   * 2. File names are not translated from one string format (UCS1, UCS2, ...)
   *    to another because it is not relevant. We make the assumption that
   *    the user gave us the file names in the current coding of his/her OS and
   *    that this coding will not change during the program execution even when
   *    glcStringType() is called.
   */

  buffer = __glcCtxQueryBuffer(state, 
			       (strlen((const char*)catalog)+1)*sizeof(char));
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  strncpy((char*)buffer, (const char*)catalog, strlen((const char*)catalog)+1);
  return buffer;
}



/** \ingroup context
 *  This command returns the integer at offset \e inIndex from the first
 *  element in the integer list identified by \e inAttrib.
 *
 *  You can choose from the following integer lists, listed below with their
 *  element count variables :
 *  <center>
 *  <table>
 *  <caption>Integer lists</caption>
 *    <tr>
 *      <td>Name</td>
 *      <td>Enumerant</td>
 *      <td>Initial value</td>
 *      <td>Element count variable</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_CURRENT_FONT_LIST</b></td>
 *      <td>0x0090</td>
 *      <td>\<empty list\></td>
 *      <td><b>GLC_CURRENT_FONT_COUNT</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FONT_LIST</b></td>
 *      <td>0x0091</td>
 *      <td>\<empty list\></td>
 *      <td><b>GLC_FONT_COUNT</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_LIST_OBJECT_LIST</b></td>
 *      <td>0x0092</td>
 *      <td>\<empty list\></td>
 *      <td><b>GLC_LIST_OBJECT_COUNT</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_TEXTURE_OBJECT_LIST</b></td>
 *      <td>0x0093</td>
 *      <td>\<empty list\></td>
 *      <td><b>GLC_TEXTURE_OBJECT_COUNT</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *
 *  The command raises a \b GLC_PARAMETER_ERROR if \e inIndex is less than zero
 *  or is greater than or equal to the value of the list's element count
 *  variable.
 *  \param inAttrib The integer list attribute
 *  \param inIndex The index from which to retrieve the element.
 *  \return the element from the integer list.
 *  \sa glcGetListc()
 */
GLint glcGetListi(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;

  /* Check parameters */
  switch(inAttrib) {
  case GLC_CURRENT_FONT_LIST:
  case GLC_FONT_LIST:
  case GLC_LIST_OBJECT_LIST:
  case GLC_TEXTURE_OBJECT_LIST:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* NOTE : at this stage we can not verify if inIndex is greater than or equal
   * to the last element index. In order to perform such a verification we
   * would need to have the current context states but GLC specs says that we
   * should first check the parameters _then_ the current context (section 2.2
   * of specs). We are done !
   */
  if (inIndex < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Perform the final part of verifications (the one which needs
   * the current context's states) then return the requested value.
   */
  inIndex--;
  switch(inAttrib) {
  case GLC_CURRENT_FONT_LIST:
    for (node = state->currentFontList.head; inIndex && node;
         node = node->next, inIndex--) {}

    if (node)
      return ((__glcFont*)node->data)->id;
    else {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
  case GLC_FONT_LIST:
    for (node = state->fontList.head; inIndex && node;
         node = node->next, inIndex--) {}

    if (node)
      return ((__glcFont*)node->data)->id;
    else {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
  case GLC_LIST_OBJECT_LIST:
    /* In order to get the display list name, we have to perform a search
     * through the list of display lists of every master. Actually we do not
     * even know which glyph of which font of which master the requested index
     * of a display list represents...
     */
    for(node = state->masterList.head; node; node = node->next) {
      FT_ListNode dlTree = NULL;

      for (dlTree = ((__glcMaster*)node->data)->displayList.head;
	   dlTree && inIndex; dlTree = dlTree->next, inIndex--);
      if (dlTree)
	return ((__glcDisplayListKey*)dlTree)->list;
    }
    return 0;
  case GLC_TEXTURE_OBJECT_LIST:
    /* See also comments of GLC_LIST_OBJECT_LIST above */
    for(node = state->masterList.head; node; node = node->next) {
      FT_ListNode texNode = NULL;

      for (texNode = ((__glcMaster*)node->data)->textureObjectList.head;
	   texNode && inIndex; texNode = texNode->next, inIndex--);
      if (!inIndex && texNode)
	return (GLint)texNode->data;
    }
    return 0;
  }

  return 0;
}



/** \ingroup context
 *  This command returns the value of the pointer variable identified by
 *  \e inAttrib.
 *  <center>
 *  <table>
 *  <caption>Pointer variables</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td> <td>Initial value</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_DATA_POINTER</b></td>
 *      <td>0x00A0</td>
 *      <td><b>GLC_NONE</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inAttrib The pointer category
 *  \return the pointer
 *  \sa glcDataPointer()
 */
GLvoid * glcGetPointer(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameter */
  if (inAttrib != GLC_DATA_POINTER) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  return state->dataPointer;
}



/** \ingroup context
 *  This command returns the value of the string constant identified by
 *  \e inAttrib. String constants must be chosen in the table below :
 *  <center>
 *  <table>
 *  <caption>String constants</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_EXTENSIONS</b></td> <td>0x00B0</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_RELEASE</b></td> <td>0x00B1</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_VENDOR</b></td> <td>0x00B2</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inAttrib The attribute that identifies the string constant
 *  \return the string constant.
 *  \sa glcGetf()
 *  \sa glcGeti()
 *  \sa glcGetfv()
 *  \sa glcGetPointer()
 */
const GLCchar* glcGetc(GLCenum inAttrib)
{
  static GLCchar* __glcExtensions = (GLCchar*) "";
  static GLCchar* __glcRelease = (GLCchar*) "Release 0.1";
  static GLCchar* __glcVendor = (GLCchar*) "Queso Software";

  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inAttrib) {
  case GLC_EXTENSIONS:
  case GLC_RELEASE:
  case GLC_VENDOR:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Get the relevant string in a Unicode string in GLC_UCS1 format */
  switch(inAttrib) {
  case GLC_EXTENSIONS:
    return __glcConvertFromUtf8ToBuffer(state, __glcExtensions, GLC_UCS1);
  case GLC_RELEASE:
    return __glcConvertFromUtf8ToBuffer(state, __glcRelease, GLC_UCS1);
  case GLC_VENDOR:
    return __glcConvertFromUtf8ToBuffer(state, __glcVendor, GLC_UCS1);
  default:
    return GLC_NONE;
  }
}



/** \ingroup context
 *  This command returns the value of the floating point variable identified
 *  by \e inAttrib.
 *  <center>
 *  <table>
 *    <caption>Float point variables</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td> <td>Initial value</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_RESOLUTION</b></td> <td>0x00C0</td> <td>0.0</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inAttrib The parameter value to be returned.
 *  \return The current value of the floating point variable.
 *  \sa glcGetc()
 *  \sa glcGeti()
 *  \sa glcGetfv()
 *  \sa glcGetPointer()
 */
GLfloat glcGetf(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameter */
  if (inAttrib != GLC_RESOLUTION) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0.f;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0.f;
  }

  return state->resolution;
}



/** \ingroup context
 *  This command stores into \e outVec the value of the floating point vector
 *  identified by \e inAttrib. If the command does not raise an error, it
 *  returns \e outVec, otherwise it returns a \b NULL value.
 *  <center>
 *  <table>
 *  <caption>Floating point vector variables</caption>
 *  <tr>
 *    <td>Name</td> <td>Enumerant</td> <td>Initial value</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_BITMAP_MATRIX</b></td> <td>0x00D0</td> <td>[ 1. 0. 0. 1.]</td>
 *  </tr>
 *  </table>
 *  </center>
 *  \param inAttrib The parameter value to be returned
 *  \param outVec Specifies where to store the return value
 *  \return the current value of the floating point vector variable
 *  \sa glcGetf()
 *  \sa glcGeti()
 *  \sa glcGetc()
 *  \sa glcGetPointer()
 */
GLfloat* glcGetfv(GLCenum inAttrib, GLfloat* outVec)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  if (inAttrib != GLC_BITMAP_MATRIX) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  memcpy(outVec, state->bitmapMatrix, 4 * sizeof(GLfloat));

  return outVec;
}



/** \ingroup context
 *  This command returns the value of the integer variable or constant
 *  identified by \e inAttrib.
 *  <center>
 *  <table>
 *  <caption>Integer variables and constants</caption>
 *  <tr>
 *    <td>Name</td> <td>Enumerant</td> <td>Initial value</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_CATALOG_COUNT</b></td>
 *    <td>0x00E0</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_CURRENT_FONT_COUNT</b></td> <td>0x00E1</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_FONT_COUNT</b></td> <td>0x00E2</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_LIST_OBJECT_COUNT</b></td> <td>0x00E3</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_MASTER_COUNT</b></td>
 *    <td>0x00E4</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_MEASURED_CHAR_COUNT</b></td> <td>0x00E5</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_RENDER_STYLE</b></td>
 *    <td>0x00E6</td>
 *    <td><b>GLC_BITMAP</b></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_REPLACEMENT_CODE</b></td> <td>0x00E7</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_STRING_TYPE</b></td> <td>0x00E8</td> <td><b>GLC_UCS1</b></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_TEXTURE_OBJECT_COUNT</b></td> <td>0x00E9</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_VERSION_MAJOR</b></td>
 *    <td>0x00EA</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_VERSION_MINOR</b></td>
 *    <td>0x00EB</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  </table>
 *  </center>
 *  \param inAttrib Attribute for which an integer variable is requested.
 *  \return the value or values of the integer variable.
 *  \sa glcGetc()
 *  \sa glcGetf()
 *  \sa glcGetfv()
 *  \sa glcGetPointer()
 */
GLint glcGeti(GLCenum inAttrib)
{
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;
  GLint count = 0;
  FcStrList* iterator = NULL;
  FcChar8* catalog = NULL;

  /* Check the parameters */
  switch(inAttrib) {
  case GLC_CATALOG_COUNT:
  case GLC_CURRENT_FONT_COUNT:
  case GLC_FONT_COUNT:
  case GLC_LIST_OBJECT_COUNT:
  case GLC_MASTER_COUNT:
  case GLC_MEASURED_CHAR_COUNT:
  case GLC_RENDER_STYLE:
  case GLC_REPLACEMENT_CODE:
  case GLC_STRING_TYPE:
  case GLC_TEXTURE_OBJECT_COUNT:
  case GLC_VERSION_MAJOR:
  case GLC_VERSION_MINOR:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Returns the requested value */
  switch(inAttrib) {
  case GLC_CATALOG_COUNT:
    __glcLock();
    iterator = FcStrListCreate(__glcCommonArea.catalogList);
    if (!iterator) {
      __glcUnlock();
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    for (catalog = FcStrListNext(iterator), count = 0; catalog;
	catalog = FcStrListNext(iterator), count++);
    FcStrListDone(iterator);
    __glcUnlock();
    return count;
  case GLC_CURRENT_FONT_COUNT:
    for (node = state->currentFontList.head, count = 0; node;
	 node = node->next, count++);
    return count;
  case GLC_FONT_COUNT:
    for (count = 0, node = state->fontList.head; node;
	 node = node->next, count++);
    return count;
  case GLC_LIST_OBJECT_COUNT:
    for (count = 0, node = state->masterList.head; node; node = node->next) {
      FT_ListNode dlTree = NULL;

      for (dlTree = ((__glcMaster*)node->data)->displayList.head; dlTree;
	   dlTree = dlTree->next, count++);
    }
    return count;
  case GLC_MASTER_COUNT:
    for (node = state->masterList.head, count = 0; node;
	 node = node->next, count++);
    return count;
  case GLC_MEASURED_CHAR_COUNT:
    return state->measuredCharCount;
  case GLC_RENDER_STYLE:
    return state->renderStyle;
  case GLC_REPLACEMENT_CODE:
    return state->replacementCode;
  case GLC_STRING_TYPE:
    return state->stringType;
  case GLC_TEXTURE_OBJECT_COUNT:
    for(count = 0, node = state->masterList.head; node; node = node->next) {
      FT_ListNode texNode = NULL;

      for (texNode = ((__glcMaster*)node->data)->textureObjectList.head;
	   texNode; texNode = texNode->next, count++);
    }
    return count;
  case GLC_VERSION_MAJOR:
    return __glcCommonArea.versionMajor;
  case GLC_VERSION_MINOR:
    return __glcCommonArea.versionMinor;
  }

  return 0;
}



/** \ingroup context
 *  This command returns \b GL_TRUE if the value of the boolean variable
 *  identified by inAttrib is \b GL_TRUE <em>(quoted from the specs ^_^)</em>
 *
 *  Attributes that can enabled and disables are listed on the glcDisable()
 *  description.
 *  \param inAttrib the attribute to be tested
 *  \return the state of the attribute \e inAttrib.
 *  \sa glcEnable()
 *  \sa glcDisable()
 */
GLboolean glcIsEnabled(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
  case GLC_GL_OBJECTS:
  case GLC_MIPMAP:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GL_FALSE;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GL_FALSE;
  }

  /* Returns the requested value */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
    return state->autoFont;
  case GLC_GL_OBJECTS:
    return state->glObjects;
  case GLC_MIPMAP:
    return state->mipmap;
  }

  return GL_FALSE;
}



/** \ingroup context
 *  This command assigns the value \e inStringType to the variable
 *  \b GLC_STRING_TYPE. The string types are listed in the table
 *  below :
 *  <center>
 *  <table>
 *  <caption>String types</caption>
 *  <tr>
 *    <td>Name</td> <td>Enumerant</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS1</b></td> <td>0x0110</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS2</b></td> <td>0x0111</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS4</b></td> <td>0x0112</td>
 *  </tr>
 *  </table>
 *  </center>
 *
 *  Every character string used in the GLC API is represented as a
 *  zero-terminated array, unless otherwise specified. The value of
 *  the variable \b GLC_STRING_TYPE determines the interpretation of
 *  the array. The values \b GLC_UCS1, \b GLC_UCS2 and \b GLC_UCS4
 *  specify that each array element is a UCS code of type \b GLubyte,
 *  \b GLushort or \b GLint, respectively. The initial value of
 *  \b GLC_STRING_TYPE is \b GLC_UCS1.
 *
 *  Some GLC commands return strings. The return value of these commands
 *  is a pointer to a string return buffer in the issuing thread's current
 *  GLC context. This pointer is valid until the next GLC command is
 *  issued. The pointer may be used as a parameter to the next GLC command.
 *  The client must copy the returned string to a client-provided buffer if
 *  the value of the string is needed after the next GLC command is issued.
 *
 *  The value of a character code in a returned string may exceed the range
 *  of the character encoding selected by \b GLC_STRING_TYPE. In this case,
 *  the returned character is converted to a character sequence
 *  <em>\\\<hexcode\></em>, where \\ is the character REVERSE SOLIDUS (U+5C),
 *  \< is the character LESS-THAN SIGN (U+3C), \> is the character
 *  GREATER-THAN SIGN (U+3E), and \e hexcode is the original character code
 *  represented as a sequence of hexadecimal digits. The sequence has no
 *  leading zeros, and alphabetic digits are in upper case.
 *  \param inStringType Value to assign to \b GLC_STRING_TYPE
 *  \sa glcGeti() with argument \b GLC_STRING_TYPE
 */
void glcStringType(GLCenum inStringType)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inStringType) {
  case GLC_UCS1:
  case GLC_UCS2:
  case GLC_UCS4:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  state->stringType = inStringType;
  return;
}
