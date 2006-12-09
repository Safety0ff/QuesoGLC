/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2006, Bertrand Coconnier
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

/** \defgroup context Context State Commands
 *  Commands to get or modify informations of the context state of the current
 *  thread.
 *
 * GLC refers to the current context state whenever it executes a command.
 * Most of its state is directly available to the user : in order to control
 * the result of the GLC commands, the user may want to get or modify the
 * state of the current context. This is precisely the purpose of the context
 * state commands.
 *
 * \note Some GLC commands create, use or delete display lists and/or textures.
 * The IDs of those display lists and textures are stored in the current GLC
 * context but the display lists and the textures themselves are managed by
 * the current \b GL context. In order not to impact the performance of
 * error-free programs, QuesoGLC does not check if the current GL context is
 * the same as the context where the display lists and the textures are
 * actually stored. If the current GL context has changed meanwhile, the result
 * of commands that refer to the corresponding display lists or textures is
 * undefined.
 */

#include "internal.h"
#include "oglyph.h"



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
 *  The callback function can access to client data in a thread-safe manner
 *  with glcGetPointer().
 *  \param inOpcode Type of the callback function
 *  \param inFunc Callback function
 *  \sa glcGetCallbackFunc()
 *  \sa glcGetPointer()
 *  \sa glcDataPointer()
 *  \sa glcRenderChar()
 */
void APIENTRY glcCallbackFunc(GLCenum inOpcode, GLCfunc inFunc)
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

  state->stringState.callback = inFunc;
}



/** \ingroup context
 *  This command assigns the value \e inPointer to the variable
 *  \b GLC_DATA_POINTER. It is used for an access to client data from the
 *  callback function assigned to the variable \b GLC_OP_glcUnmappedCode
 *
 *  \e glcDataPointer provides a way to store, in the GLC context, a pointer to
 *  any data. A GLC callback function can subsequently use the command
 *  glcGetPointer() to obtain access to those data in a thread-safe manner.
 *  \param inPointer The pointer to assign to \b GLC_DATA_POINTER
 *  \sa glcGetPointer()
 *  \sa glcCallbackFunc()
 */
void APIENTRY glcDataPointer(GLvoid *inPointer)
{
  __glcContextState *state = NULL;

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  state->stringState.dataPointer = inPointer;
}



/** \ingroup context
 *  This command causes GLC to issue a sequence of GL commands to delete all
 *  of the GL objects it owns.
 *
 *  GLC uses the command \c glDeleteLists to delete all of the GL objects named
 *  in \b GLC_LIST_OBJECT_LIST and uses the command \c glDeleteTextures to
 *  delete all of the GL objects named in \b GLC_TEXTURE_OBJECT_LIST. When an
 *  execution of glcDeleteGLObjects finishes, both of these lists are empty.
 *  \note \c glcDeleteGLObjects deletes only the objects that the current
 *  GLC context owns, not all objects in all contexts.
 *  \sa glcGetListi()
 */
void APIENTRY glcDeleteGLObjects(void)
{
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;

  /* Check if the thread has a current context */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* The GL objects are managed by the __glcFaceDescriptor object. Hence we
   * parse the __glcFaceDescriptor stored in each __glcMaster.
   */
  for(node = state->masterList.head; node; node = node->next) {
    FT_ListNode faceNode = ((__glcMaster*)node)->faceList.head;
    for (; faceNode; faceNode = faceNode->next)
      __glcFaceDescDestroyGLObjects((__glcFaceDescriptor*)faceNode);
  }

  /* Delete the texture used for immediate mode */
  if (state->texture.id) {
    glDeleteTextures(1, &state->texture.id);
    state->texture.id = 0;
    state->texture.width = 0;
    state->texture.heigth = 0;
  }
}



/* This internal function is used by both glcEnable/glcDisable since they
 * actually do the same job : put a value into a member of the
 * __glcContextState struct. The only difference is the value that it puts.
 */
static void __glcChangeState(GLCenum inAttrib, GLboolean value)
{
  __glcContextState *state = NULL;

  /* Check the parameters. */
  assert((value == GL_TRUE) || (value == GL_FALSE));

  switch(inAttrib) {
  case GLC_AUTO_FONT:
  case GLC_GL_OBJECTS:
  case GLC_MIPMAP:
  case GLC_HINTING_QSO: /* QuesoGLC Extension */
  case GLC_EXTRUDE_QSO: /* QuesoGLC Extension */
  case GLC_KERNING_QSO: /* QuesoGLC Extension */
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
    state->enableState.autoFont = value;
    break;
  case GLC_GL_OBJECTS:
    state->enableState.glObjects = value;
    break;
  case GLC_MIPMAP:
    state->enableState.mipmap = value;
    if (state->atlas.id) {
      GLint boundTexture = 0;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
      glBindTexture(GL_TEXTURE_2D, state->atlas.id);
      if (state->enableState.mipmap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
      else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, boundTexture);
    }
    break;
  case GLC_HINTING_QSO:
    state->enableState.hinting = value;
    break;
  case GLC_EXTRUDE_QSO:
    state->enableState.extrude = value;
    break;
  case GLC_KERNING_QSO:
    state->enableState.kerning = value;
    break;
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
 *    <tr>
 *      <td><b>GLC_HINTING_QSO</b></td>
 *      <td>0x8005</td>
 *      <td><b>GL_FALSE</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_EXTRUDE_QSO</b></td>
 *      <td>0x8006</td>
 *      <td><b>GL_FALSE</b></td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_KERNING_QSO</b></td>
 *      <td>0x8007</td>
 *      <td><b>GL_FALSE</b></td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inAttrib A symbolic constant indicating a GLC capability.
 *  \sa glcIsEnabled()
 *  \sa glcEnable()
 */
void APIENTRY glcDisable(GLCenum inAttrib)
{
  __glcChangeState(inAttrib, GL_FALSE);
}



/** \ingroup context
 *  This command assigns the value \b GL_TRUE to the boolean variable
 *  identified by \e inAttrib which must be chosen in the table above.
 *
 *  - \b GLC_AUTO_FONT : if enabled, GLC tries to automatically find a font
 *    among the masters to map the character code to be rendered (see also
 *    glcRenderChar()).
 *  - \b GLC_GL_OBJECTS : if enabled, GLC stores characters rendering commands
 *    in GL display lists and textures (if any) in GL texture objects.
 *  - \b GLC_MIPMAP : if enabled, texture objects used by GLC are mipmapped
 *  - \b GLC_HINTING_QSO : if enabled, GLC uses the auto-hinting procedures
 *    that are available for most scalable fonts. It gives better results for
 *    characters that are rendered at small sizes. Hinting may however generate
 *    visual artifacts such as shaking outlines if the character is animated.
 *    This attribute should be disabled in such cases.
 *  - \b GLC_EXTRUDE_QSO : if enabled and \b GLC_RENDER_STYLE is
 *    \b GLC_TRIANGLE then GLC renders extruded characters with a thickness
 *    equal to 1.0. A call to glScale3*(1., 1., \e thickness ) can be added
 *    before the rendering commands in order to obtain the desired thickness.
 *  - \b GLC_KERNING_QSO : if enabled, GLC uses kerning information (when
 *    available) when string are rendered or measured.
 *
 *  \param inAttrib A symbolic constant indicating a GLC attribute.
 *  \sa glcDisable()
 *  \sa glcIsEnabled()
 */
void APIENTRY glcEnable(GLCenum inAttrib)
{
  __glcChangeState(inAttrib, GL_TRUE);
}



/** \ingroup context
 *  This command returns the value of the callback function variable identified
 *  by \e inOpcode. Currently, \e inOpcode can only have the value
 *  \b GLC_OP_glcUnmappedCode. Its initial value and the type signature are
 *  defined in the table shown in glcCallbackFunc()'s definition.
 *  \param inOpcode The callback function to be retrieved
 *  \return The value of the callback function variable
 *  \sa glcCallbackFunc()
 */
GLCfunc APIENTRY glcGetCallbackFunc(GLCenum inOpcode)
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

  return state->stringState.callback;
}



/** \ingroup context
 *  This command returns the string at offset \e inIndex from the first element
 *  in the string list identified by \e inAttrib which must be chosen in the
 *  table below :
 *
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
 *  \return The string list element
 *  \sa glcGetListi()
 */
const GLCchar* APIENTRY glcGetListc(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  FcStrList* iterator = NULL;
  FcChar8* catalog = NULL;
  GLCchar* buffer = NULL;
  int length = 0;

  /* Check the parameters */
  if (inAttrib != GLC_CATALOG_LIST) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* NOTE : at this stage we can not verify if inIndex is greater than or equal
   * to the last element index. In order to perform such a verification we
   * would need to have the current context state but GLC specs tells that we
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
  iterator = FcStrListCreate(state->catalogList);
  if (!iterator) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  for (catalog = FcStrListNext(iterator); catalog && inIndex;
	catalog = FcStrListNext(iterator), inIndex--);
  FcStrListDone(iterator);

  /* Now we can check if inIndex identifies a member of the string set or 
   * not */
  if (!catalog) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Three remarks have to be made concerning the following code :
   * 1. We do not return a pointer that points to the actual location of the
   *    string in order to prevent the user to modify it. Instead QuesoGLC
   *    returns a pointer that points to a copy of the requested data.
   * 2. File names are not translated from one string format (UCS1, UCS2, ...)
   *    to another because it is not relevant. We make the assumption that
   *    the user gave us the file names in the current coding of his/her OS and
   *    that this coding will not change during the program execution even when
   *    glcStringType() is called.
   * 3. There are few chances that this code is executed in a critical loop
   *    then there is no need to care about optimization.
   */

  /* Grmmff, is the use of strlen() adequate here ? 
   * What if 'catalog' is encoded in UCS2 format ?
   */
  length = strlen((const char*) catalog) + 1;

  buffer = __glcCtxQueryBuffer(state, length * sizeof(char));
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  strncpy((char*)buffer, (const char*)catalog, length);
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
 *  \return The element from the integer list.
 *  \sa glcGetListc()
 */
GLint APIENTRY glcGetListi(GLCenum inAttrib, GLint inIndex)
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
  switch(inAttrib) {
  case GLC_CURRENT_FONT_LIST:
    for (node = state->currentFontList.head; inIndex && node;
         node = node->next, inIndex--);

    if (node)
      return ((__glcFont*)node->data)->id;
    else {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
  case GLC_FONT_LIST:
    for (node = state->fontList.head; inIndex && node;
         node = node->next, inIndex--);

    if (node)
      return ((__glcFont*)node->data)->id;
    else {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
  case GLC_LIST_OBJECT_LIST:
    /* In order to get the display list name, we have to perform a search
     * through the list of display lists of every face descriptor.
     */
    for (node = state->masterList.head; node; node = node->next) {
      __glcMaster* master = (__glcMaster*)node;
      FT_ListNode faceNode = NULL;

      for (faceNode = master->faceList.head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)faceNode;
	FT_ListNode glyphNode = NULL;

	for (glyphNode = faceDesc->glyphList.head; glyphNode;
	     glyphNode = glyphNode->next) {
	  __glcGlyph* glyph = (__glcGlyph*)glyphNode;
	  int count = __glcGlyphGetDisplayListCount(glyph);

	  if (inIndex < count)
	    return __glcGlyphGetDisplayList(glyph, inIndex);
	  else
	    inIndex -= count;
	}
      }
    }
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  case GLC_TEXTURE_OBJECT_LIST:
    switch(inIndex) {
      case 1:
	if (state->texture.id)
	  return state->texture.id;
	if (state->atlas.id)
	  return state->atlas.id;
	break;
      case 2:
	if ((state->texture.id) && (state->atlas.id))
	  return state->atlas.id;
	break;
      default:
	break;
    }
    __glcRaiseError(GLC_PARAMETER_ERROR);
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
 *  \return The pointer
 *  \sa glcDataPointer()
 */
GLvoid* APIENTRY glcGetPointer(GLCenum inAttrib)
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

  return state->stringState.dataPointer;
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
 *  \return The string constant.
 *  \sa glcGetf()
 *  \sa glcGeti()
 *  \sa glcGetfv()
 *  \sa glcGetPointer()
 */
const GLCchar* APIENTRY glcGetc(GLCenum inAttrib)
{
  static GLCchar* __glcExtensions = (GLCchar*) "GLC_QSO_utf8 GLC_SGI_full_name"
    " GLC_QSO_hinting GLC_QSO_extrude GLC_QSO_kerning GLC_QSO_matrix_stack"
    " GLC_QSO_attrib_stack";
  static GLCchar* __glcVendor = (GLCchar*) "The QuesoGLC Project";
  static GLCchar* __glcRelease = (GLCchar*) QUESOGLC_VERSION;

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

  /* Translate the string to the relevant Unicode format */
  switch(inAttrib) {
  case GLC_EXTENSIONS:
    return __glcConvertFromUtf8ToBuffer(state, __glcExtensions,
					state->stringState.stringType);
  case GLC_RELEASE:
    return __glcConvertFromUtf8ToBuffer(state, (GLCchar*)__glcRelease,
					state->stringState.stringType);
  case GLC_VENDOR:
    return __glcConvertFromUtf8ToBuffer(state, __glcVendor,
					state->stringState.stringType);
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
GLfloat APIENTRY glcGetf(GLCenum inAttrib)
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

  return state->renderState.resolution;
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
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e outVec is NULL.
 *  \param inAttrib The parameter value to be returned
 *  \param outVec Specifies where to store the return value
 *  \return The current value of the floating point vector variable
 *  \sa glcGetf()
 *  \sa glcGeti()
 *  \sa glcGetc()
 *  \sa glcGetPointer()
 */
GLfloat* APIENTRY glcGetfv(GLCenum inAttrib, GLfloat* outVec)
{
  __glcContextState *state = NULL;

  assert(outVec);

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
 *  <tr>
 *    <td><b>GLC_MATRIX_STACK_DEPTH_QSO</b></td> <td>0x8008</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_MAX_MATRIX_STACK_DEPTH_QSO</b></td>
 *    <td>0x8009</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_ATTRIB_STACK_DEPTH_QSO</b></td> <td>0x800C</td> <td>0</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_MAX_ATTRIB_STACK_DEPTH_QSO</b></td>
 *    <td>0x800D</td>
 *    <td>\<implementation specific\></td>
 *  </tr>
 *  </table>
 *  </center>
 *  \param inAttrib Attribute for which an integer variable is requested.
 *  \return The value or values of the integer variable.
 *  \sa glcGetc()
 *  \sa glcGetf()
 *  \sa glcGetfv()
 *  \sa glcGetPointer()
 */
GLint APIENTRY glcGeti(GLCenum inAttrib)
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
  case GLC_MATRIX_STACK_DEPTH_QSO:     /* QuesoGLC extension */
  case GLC_MAX_MATRIX_STACK_DEPTH_QSO: /* QuesoGLC extension */
  case GLC_ATTRIB_STACK_DEPTH_QSO:     /* QuesoGLC extension */
  case GLC_MAX_ATTRIB_STACK_DEPTH_QSO: /* QuesoGLC extension */
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

  /* Returns the requested value */
  switch(inAttrib) {
  case GLC_CATALOG_COUNT:
    iterator = FcStrListCreate(state->catalogList);
    if (!iterator) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }
    for (catalog = FcStrListNext(iterator), count = 0; catalog;
	catalog = FcStrListNext(iterator), count++);
    FcStrListDone(iterator);
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
    for (node = state->masterList.head; node; node = node->next) {
      __glcMaster* master = (__glcMaster*)node;
      FT_ListNode faceNode = NULL;

      for (faceNode = master->faceList.head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)faceNode;
	FT_ListNode glyphNode = NULL;

	for (glyphNode = faceDesc->glyphList.head; glyphNode;
	     glyphNode = glyphNode->next) {
	  __glcGlyph* glyph = (__glcGlyph*)glyphNode;

	  count += __glcGlyphGetDisplayListCount(glyph);
	}
      }
    }
    return count;
  case GLC_MASTER_COUNT:
    for (node = state->masterList.head, count = 0; node;
	 node = node->next, count++);
    return count;
  case GLC_MEASURED_CHAR_COUNT:
    return GLC_ARRAY_LENGTH(state->measurementBuffer);
  case GLC_RENDER_STYLE:
    return state->renderState.renderStyle;
  case GLC_REPLACEMENT_CODE:
    /* Return the replacement character converted to the current string type */
    return __glcConvertUcs4ToGLint(state, state->stringState.replacementCode);
  case GLC_STRING_TYPE:
    return state->stringState.stringType;
  case GLC_TEXTURE_OBJECT_COUNT:
    count += (state->texture.id ? 1: 0);
    count += (state->atlas.id ? 1:0);
    return count;
  case GLC_VERSION_MAJOR:
    return __glcCommonArea.versionMajor;
  case GLC_VERSION_MINOR:
    return __glcCommonArea.versionMinor;
  case GLC_MATRIX_STACK_DEPTH_QSO:     /* QuesoGLC extension */
    return state->bitmapMatrixStackDepth;
  case GLC_MAX_MATRIX_STACK_DEPTH_QSO: /* QuesoGLC extension */
    return GLC_MAX_MATRIX_STACK_DEPTH;
  case GLC_ATTRIB_STACK_DEPTH_QSO:     /* QuesoGLC extension */
    return state->attribStackDepth;
  case GLC_MAX_ATTRIB_STACK_DEPTH_QSO: /* QuesoGLC extension */
    return GLC_MAX_ATTRIB_STACK_DEPTH;
  }

  return 0;
}



/** \ingroup context
 *  This command returns \b GL_TRUE if the value of the boolean variable
 *  identified by inAttrib is \b GL_TRUE <em>(quoted from the specs ^_^)</em>
 *
 *  Attributes that can be enabled or disabled are listed on the glcDisable()
 *  description.
 *  \param inAttrib The attribute to be tested
 *  \return The state of the attribute \e inAttrib.
 *  \sa glcEnable()
 *  \sa glcDisable()
 */
GLboolean APIENTRY glcIsEnabled(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
  case GLC_GL_OBJECTS:
  case GLC_MIPMAP:
  case GLC_HINTING_QSO: /* QuesoGLC Extension */
  case GLC_EXTRUDE_QSO: /* QuesoGLC Extension */
  case GLC_KERNING_QSO: /* QuesoGLC Extension */
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
    return state->enableState.autoFont;
  case GLC_GL_OBJECTS:
    return state->enableState.glObjects;
  case GLC_MIPMAP:
    return state->enableState.mipmap;
  case GLC_HINTING_QSO:
    return state->enableState.hinting;
  case GLC_EXTRUDE_QSO:
    return state->enableState.extrude;
  case GLC_KERNING_QSO:
    return state->enableState.kerning;
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
 *    <td>Name</td> <td>Enumerant</td> <td>Type of characters</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS1</b></td> <td>0x0110</td> <td>GLubyte</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS2</b></td> <td>0x0111</td> <td>GLushort</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UCS4</b></td> <td>0x0112</td> <td>GLuint</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_UTF8_QSO</b></td>
 *    <td>0x8004</td>
 *    <td>\<character dependent\></td>
 *  </tr>
 *  </table>
 *  </center>
 *
 *  Every character string used in the GLC API is represented as a
 *  zero-terminated array, unless otherwise specified. The value of the
 *  variable \b GLC_STRING_TYPE determines the interpretation of the array. The
 *  values \b GLC_UCS1, \b GLC_UCS2, \b GLC_UCS4 and \b GLC_UTF8_QSO indicate
 *  how each element of the string should be interpreted. Currently QuesoGLC
 *  supports UCS1, UCS2, UCS4 and UTF-8 formats as defined in the Unicode 4.0.1
 *  and ISO/IEC 10646:2003 standards. The initial value of \b GLC_STRING_TYPE
 *  is \b GLC_UCS1.
 *
 *  \note Currently, the string formats UCS2 and UCS4 are interpreted according
 *  to the underlying platform endianess. If the strings are provided in a
 *  different endianess than the platform's, the client must translate the
 *  strings in the correct endianess.
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
void APIENTRY glcStringType(GLCenum inStringType)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  switch(inStringType) {
  case GLC_UCS1:
  case GLC_UCS2:
  case GLC_UCS4:
  case GLC_UTF8_QSO: /* QuesoGLC Extension */
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

  state->stringState.stringType = inStringType;
  return;
}



/** \ingroup context
 *  This command provides a means to save groups of state variables. It takes a
 *  OR of symbolic constants indicating which groups of state variables to push
 *  onto the attribute stack. Each constant refers to a group of state
 *  variables.
 *  <center>
 *  <table>
 *  <caption>Group attributes</caption>
 *  <tr>
 *    <td>Group attribute</td> <td>Name</td>
 *  </tr>
 *  <tr>
 *    <td>enable</td> <td><b>GLC_ENABLE_BIT_QSO</b></td>
 *  </tr>
 *  <tr>
 *    <td>render</td> <td><b>GLC_RENDER_BIT_QSO</b></td>
 *  </tr>
 *  <tr>
 *    <td>string</td> <td><b>GLC_STRING_BIT_QSO</b></td>
 *  </tr>
 *  <tr>
 *    <td> </td> <td><b>GLC_GL_ATTRIB_BIT_QSO</b></td>
 *  </tr>
 *  <tr>
 *    <td> </td> <td><b>GLC_ALL_ATTRIBS_BIT_QSO</b></td>
 *  </tr>
 *  </table>
 *  </center>
 *
 *  The classification of each variable into a group is indicated in the
 *  following table of state variables.
 *  <center>
 *  <table>
 *  <caption>State variables</caption>
 *  <tr>
 *    <td>Name</td> <td>Type</td> <td>Get command</td> <td>Group attribute</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_AUTO_FONT</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_GL_OBJECTS</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_MIPMAP</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_HINTING_QSO</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_EXTRUDE_QSO</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
  *  <tr>
 *    <td><b>GLC_KERNING_QSO</b></td>
 *    <td>GLboolean</td>
 *    <td>glcIsEnabled()</td>
 *    <td>enable</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_RENDER_STYLE</b></td>
 *    <td>GLint</td>
 *    <td>glcGeti()</td>
 *    <td>render</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_RESOLUTION</b></td>
 *    <td>GLfloat</td>
 *    <td>glcGetf()</td>
 *    <td>render</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_STRING_TYPE</b></td>
 *    <td>GLint</td>
 *    <td>glcGeti()</td>
 *    <td>string</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_REPLACEMENT_CODE</b></td>
 *    <td>GLint</td>
 *    <td>glcGeti()</td>
 *    <td>string</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_OP_glcUnmappedCode</b></td>
 *    <td>GLCfunc</td>
 *    <td>glcGetCallbackFunc()</td>
 *    <td>string</td>
 *  </tr>
 *  <tr>
 *    <td><b>GLC_DATA_POINTER</b></td>
 *    <td>GLvoid*</td>
 *    <td>glcGetPointer()</td>
 *    <td>string</td>
 *  </tr>
 *  </table>
 *  </center>
 *
 *  The error \b GLC_STACK_OVERFLOW_QSO is generated if glcPushAttribQSO() is
 *  executed while the attribute stack depth is equal to
 *  \b GLC_MAX_ATTRIB_STACK_DEPTH_QSO.
 *  \b GLC_STACK_OVERFLOW_QSO.
 *  \sa glcPopAttribQSO()
 *  \sa glcGeti() with argument \b GLC_ATTRIB_STACK_DEPTH_QSO
 *  \sa glcGeti() with argument \b GLC_MAX_ATTRIB_STACK_DEPTH_QSO
 */
void APIENTRY glcPushAttribQSO(GLbitfield inMask)
{
  __glcContextState *state = NULL;
  __glcAttribStackLevel *level = NULL;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  if (state->attribStackDepth >= GLC_MAX_ATTRIB_STACK_DEPTH) {
    __glcRaiseError(GLC_STACK_OVERFLOW_QSO);
    return;
  }

  level = &state->attribStack[state->attribStackDepth++];
  level->attribBits = 0;

  if (inMask & GLC_ENABLE_BIT_QSO) {
    memcpy(&level->enableState, &state->enableState, sizeof(__glcEnableState));
    level->attribBits |= GLC_ENABLE_BIT_QSO;
  }

  if (inMask & GLC_RENDER_BIT_QSO) {
    memcpy(&level->renderState, &state->renderState, sizeof(__glcRenderState));
    level->attribBits |= GLC_RENDER_BIT_QSO;
  }

  if (inMask & GLC_STRING_BIT_QSO) {
    memcpy(&level->stringState, &state->stringState, sizeof(__glcStringState));
    level->attribBits |= GLC_STRING_BIT_QSO;
  }

  if (inMask & GLC_GL_ATTRIB_BIT_QSO) {
    __glcSaveGLState(&level->glState);
    level->attribBits |= GLC_GL_ATTRIB_BIT_QSO;
  }

  return;
}



/** \ingroup context
 *  This command resets the values of those state variables that were saved with
 *  the last corresponding glcPushAttribQSO(). Those not saved remain unchanged.
 *  The error \b GLC_STACK_UNDERFLOW_QSO is generated if glcPopAttrib() is
 *  executed while the attribute stack is empty.
 *  \sa glcPushAttribQSO()
 *  \sa glcGeti() with argument \b GLC_ATTRIB_STACK_DEPTH_QSO
 *  \sa glcGeti() with argument \b GLC_MAX_ATTRIB_STACK_DEPTH_QSO
 */
void APIENTRY glcPopAttribQSO(void)
{
  __glcContextState *state = NULL;
  __glcAttribStackLevel *level = NULL;
  GLbitfield mask;

  /* Check if the current thread owns a context state */
  state = __glcGetCurrent();
  if (!state) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  if (state->attribStackDepth <= 0) {
    __glcRaiseError(GLC_STACK_UNDERFLOW_QSO);
    return;
  }

  level = &state->attribStack[--state->attribStackDepth];
  mask = level->attribBits;

  if (mask & GLC_ENABLE_BIT_QSO)
    memcpy(&state->enableState, &level->enableState, sizeof(__glcEnableState));

  if (mask & GLC_RENDER_BIT_QSO)
    memcpy(&state->renderState, &level->renderState, sizeof(__glcRenderState));

  if (mask & GLC_STRING_BIT_QSO)
    memcpy(&state->stringState, &level->stringState, sizeof(__glcStringState));

  if (mask & GLC_GL_ATTRIB_BIT_QSO)
    __glcRestoreGLState(&level->glState);

  return;
}
