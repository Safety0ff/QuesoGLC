/* $Id$ */
#include <stdlib.h>
#include <string.h>

#include "GL/glc.h"
#include "internal.h"

/* glcCallbackFunc:
 *   This command assigns the value inFunc to the callback function variable
 *   identified by inOpCode.
 */
void glcCallbackFunc(GLCenum inOpcode, GLCfunc inFunc)
{
  __glcContextState *state = NULL;

  /* Check parameters */
  if (inOpcode != GLC_OP_glcUnmappedCode) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  state->callback = inFunc;
}

/* glcDataPointer:
 *   This command assigns the value inPointer to the variable GLC_DATA_POINTER
 */
void glcDataPointer(GLvoid *inPointer)
{
  __glcContextState *state = NULL;

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  state->dataPointer = inPointer;
}

/* glcDeleteGLObjects:
 *   This command causes GLC to issue a sequence of GL commands to delete all
 *   of the GL objects that it owns. GLC uses the command glDeleteLists to
 *   delete all of the GL objects named in GLC_LIST_OBJECT_LIST and uses the
 *   command glDeleteTextures to delete all of the GL objects named in
 *   GLC_TEXTURE_OBJECT_LIST. When an execution of glcDeleteGLObjects finishes,
 *   both of these lists are empty.
 */
void glcDeleteGLObjects(void)
{
  __glcContextState *state = NULL;
  GLint i = 0;

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  /* Deletes display lists */
  for (i = 0; i < state->masterCount; i++) {
    if (state->masterList[i])
      delete state->masterList[i]->displayList;
  }

  /* Deletes texture objects */
  glDeleteTextures(state->textureObjectCount, state->textureObjectList);

  /* Empties both GLC_LIST_OBJECT_LIST and GLC_TEXTURE_OBJECT_LIST */
  state->listObjectCount = 0;
  state->textureObjectCount = 0;
}

/* This internal function is used by both glcEnable/glcDisable since they
 * actually do the same job : put a value into a member of the
 * __glcContextState struct. The only difference is the value that it puts.
 */
static void __glcDisable(GLCenum inAttrib, GLboolean value)
{
  __glcContextState *state = NULL;

  /* Check the parameters. Note that we do not need to check 'value' since
   * it has been generated internally.
   */
  switch(inAttrib) {
  case GLC_AUTO_FONT:
  case GLC_GL_OBJECTS:
  case GLC_MIPMAP:
    break;
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
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

/* glcDisable:
 *   This command assigns the value GL_FALSE to the boolean variable identified
 *   by inAttrib.
 */
void glcDisable(GLCenum inAttrib)
{
  __glcDisable(inAttrib, GL_FALSE);
}

/* glcDisable:
 *   This command assigns the value GL_TRUE to the boolean variable identified
 *   by inAttrib.
 */
void glcEnable(GLCenum inAttrib)
{
  __glcDisable(inAttrib, GL_TRUE);
}

/* glcGetCallbackFunc:
 *   This command returns the value of the callback function variable
 *   identified by inOpcode
 */
GLCfunc glcGetCallbackFunc(GLCenum inOpcode)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  if (inOpcode != GLC_OP_glcUnmappedCode) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  return state->callback;
}

/* glcGetListc:
 *   This command returns the string at offset inIndex from the first element
 *   in the string list identified by inAttrib. The command raises a
 *   GLC_PARAMETER_ERROR if inIndex is less than zero or is greater than or
 *   equal to the value of the list's element count variable.
 */
const GLCchar* glcGetListc(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  __glcUniChar *s = NULL;
  GLCchar* buffer = NULL;

  /* Check the parameters */
  if (inAttrib != GLC_CATALOG_LIST) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Notice that at this stage we can not verify if inIndex is greater than
   * or equal to the last element index. In order to perform such a
   * verification we would need to have the current context states but GLC
   * specs says that we should first check the parameters _then_ the current
   * context (section 2.2 of specs). We are done !
   */
  if (inIndex < 0) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Now we can check if inIndex identifies a member of the string list or 
   * not */
  if ((inIndex < 0) || (inIndex >= state->catalogList->getCount())) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Two remarks have to be made concerning the following code :
   * 1. We do not return a pointer that points to the actual location of the
   *    string in order to prevent the user to modify it. Instead QuesoGLC
   *    returns a pointer that points to a copy of the requested data.
   * 2. File names are not translated from one string format (UCS1, UCS2, ...)
   *    to another because it is not relevant. We make the assumption that
   *    the user gave us the file names in the current coding used by his/her
   *    OS and that this coding does not change during the program execution
   *    even when glcStringType() is called.
   */

  /* Get the string at offset inIndex */
  s = state->catalogList->findIndex(inIndex);
  /* Allocate a buffer to store the string */
  buffer = state->queryBuffer(s->lenBytes());

  if (buffer)
    /* Copy the string into the buffer */
    s->dup(buffer, s->lenBytes());
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return buffer;
}

/* glcGetListi:
 *   This command returns the integer at offset inIndex from the first element
 *   in the integer list identified by inAttrib. The command raises a
 *   GLC_PARAMETER_ERROR if inIndex is less than zero or is greater than or
 *   equal to the value of the list's element count variable.
 */
GLint glcGetListi(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  GLint i = 0;
  BSTree *dlTree = NULL;
  GLuint *dlName = NULL;

  /* Check parameters */
  switch(inAttrib) {
  case GLC_CURRENT_FONT_LIST:
  case GLC_FONT_LIST:
  case GLC_LIST_OBJECT_LIST:
  case GLC_TEXTURE_OBJECT_LIST:
    break;
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Notice that at this stage we can not verify if inIndex is greater than
   * or equal to the last element index. In order to perform such a
   * verification we would need to have the current context states but GLC
   * specs says that we should first check the parameters _then_ the current
   * context (section 2.2 of specs). We are done !
   */
  if (inIndex < 0) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Perform the final part of verifications (the one which need to get
   * the current context's states) then return the requested value.
   */
  switch(inAttrib) {
  case GLC_CURRENT_FONT_LIST:
    if (inIndex > state->currentFontCount) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    return state->currentFontList[inIndex];
  case GLC_FONT_LIST:
    if (inIndex > state->fontCount) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    /* Verify that the requested font still exists. If it does then return its
     * ID, otherwise return zero.
     */
    return (state->fontList[inIndex] ? inIndex : 0);
  case GLC_LIST_OBJECT_LIST:
    if (inIndex > state->listObjectCount) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    /* In order to get the display list name, we have to perform a search
     * through the list of display lists of every master. Actually we do not
     * know which glyph of which font of which master the requested index
     * of a display list represents...
     */
    for (i = 0; i < state->masterCount; i++) {
      dlTree = state->masterList[i]->displayList;
      if (dlTree) {
	dlName = (GLuint *)dlTree->element(inIndex);
	if (dlName)
	  break;
      }
    }
    return *dlName;
  case GLC_TEXTURE_OBJECT_LIST:
    if (inIndex > state->textureObjectCount) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return 0;
    }
    return state->textureObjectList[inIndex];
  }

  return 0;
}

/* glcGetPointer:
 *   This command returns the value of the pointer variable identified by
 *   inAttrib.
 */
GLvoid * glcGetPointer(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameter */
  if (inAttrib != GLC_DATA_POINTER) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  return state->dataPointer;
}

/* glcGetc:
 *   This command returns the value of the string constant identified by
 *   inAttrib.
 */
const GLCchar* glcGetc(GLCenum inAttrib)
{
  static const GLCchar* __glcExtensions = "";
  static const GLCchar* __glcRelease = "draft Linux 2.x";
  static const GLCchar* __glcVendor = "Queso Software";

  __glcContextState *state = NULL;
  __glcUniChar s;
  GLCchar *buffer = NULL;
  int length = 0;

  /* Check the parameters */
  switch(inAttrib) {
  case GLC_EXTENSIONS:
  case GLC_RELEASE:
  case GLC_VENDOR:
    break;
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  /* Get the relevant string in Unicode string in GLC_UCS1 format */
  switch(inAttrib) {
  case GLC_EXTENSIONS:
    s = __glcUniChar(__glcExtensions, GLC_UCS1);
    break;
  case GLC_RELEASE:
    s = __glcUniChar(__glcRelease, GLC_UCS1);
    break;
  case GLC_VENDOR:
    s = __glcUniChar(__glcVendor, GLC_UCS1);
    break;
  default:
    return GLC_NONE;
  }
    
  /* Allocates a buffer which size equals the length of the transformed
   * string */
  length = s.estimate(state->stringType);
  buffer = state->queryBuffer(length);
  if (buffer)
    /* Converts the string into the current string type */
    s.convert(buffer, state->stringType, length);
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return buffer;
}

/* glcGetf:
 *   This command returns the value of the floating point variable identified
 *   by inAttrib.
 */
GLfloat glcGetf(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

  /* Check the parameter */
  if (inAttrib != GLC_RESOLUTION) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return 0.;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0.;
  }

  return state->resolution;
}

/* glcGetfv:
 *   This command stores into outVec the value of the floating point vector
 *   identified by inAttrib. If the command does not raise an error, it returns
 *   outVec, otherwise it returns a NULL value.
 */
GLfloat* glcGetfv(GLCenum inAttrib, GLfloat* outVec)
{
  __glcContextState *state = NULL;

  /* Check the parameters */
  if (inAttrib != GLC_BITMAP_MATRIX) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }
    
  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return NULL;
  }

  memcpy(outVec, state->bitmapMatrix, 4 * sizeof(GLfloat));

  return outVec;
}

/* glcGeti:
 *   This command returns the value of the integer variable or constant
 *   identified by inAttrib.
 */
GLint glcGeti(GLCenum inAttrib)
{
  __glcContextState *state = NULL;

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
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* Returns the requested value */
  switch(inAttrib) {
  case GLC_CATALOG_COUNT:
    return state->catalogList->getCount();
  case GLC_CURRENT_FONT_COUNT:
    return state->currentFontCount;
  case GLC_FONT_COUNT:
    return state->fontCount;
  case GLC_LIST_OBJECT_COUNT:
    return state->listObjectCount;
  case GLC_MASTER_COUNT:
    return state->masterCount;
  case GLC_MEASURED_CHAR_COUNT:
    return state->measuredCharCount;
  case GLC_RENDER_STYLE:
    return state->renderStyle;
  case GLC_REPLACEMENT_CODE:
    return state->replacementCode;
  case GLC_STRING_TYPE:
    return state->stringType;
  case GLC_TEXTURE_OBJECT_COUNT:
    return state->textureObjectCount;
  case GLC_VERSION_MAJOR:
    return state->versionMajor;
  case GLC_VERSION_MINOR:
    return state->versionMinor;
  }

  return 0;
}

/* glcIsEnabled:
 *   This command returns GL_TRUE if the value of the boolean variable
 *   identified by inAttrib is GL_TRUE (quoted from the specs ^_^)
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
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GL_FALSE;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
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

/* glcStringType:
 *   This command assigns the value inStringType to the variable
 *   GLC_STRING_TYPE.
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
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the thread has a current context */
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  state->stringType = inStringType;
  return;
}
