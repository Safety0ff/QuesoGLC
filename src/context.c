/* $Id$ */
#include <stdlib.h>
#include <string.h>

#include "GL/glc.h"
#include "internal.h"

void glcCallbackFunc(GLCenum inOpcode, GLCfunc inFunc)
{
    __glcContextState *state = NULL;
    
    if (inOpcode != GLC_OP_glcUnmappedCode) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }
    
    state->callback = inFunc;
}

void glcDataPointer(GLvoid *inPointer)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->dataPointer = inPointer;
}

void glcDeleteGLObjects(void)
{
    __glcContextState *state = NULL;
    int i = 0;

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

    state->listObjectCount = 0;
    state->textureObjectCount = 0;
}

static void __glcDisable(GLCenum inAttrib, GLboolean value)
{
    __glcContextState *state = NULL;

    switch(inAttrib) {
	case GLC_AUTO_FONT:
	case GLC_GL_OBJECTS:
	case GLC_MIPMAP:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

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

void glcDisable(GLCenum inAttrib)
{
    __glcDisable(inAttrib, GL_FALSE);
}

void glcEnable(GLCenum inAttrib)
{
    __glcDisable(inAttrib, GL_TRUE);
}

GLCfunc glcGetCallbackFunc(GLCenum inOpcode)
{
    __glcContextState *state = NULL;

    if (inOpcode != GLC_OP_glcUnmappedCode) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return GLC_NONE;
    }

    return state->callback;
}

const GLCchar* glcGetListc(GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  __glcUniChar *s = NULL;
  GLCchar* buffer = NULL;

  if (inAttrib != GLC_CATALOG_LIST) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
    
  if (inIndex < 0) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
    
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  if ((inIndex < 0) || (inIndex >= state->catalogList->getCount())) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
  
  s = state->catalogList->findIndex(inIndex);
  buffer = state->queryBuffer(s->lenBytes());
  if (buffer)
    s->dup(buffer, s->lenBytes());
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return buffer;
}

GLint glcGetListi(GLCenum inAttrib, GLint inIndex)
{
    __glcContextState *state = NULL;
    int i = 0;
    BSTree *dlTree = NULL;
    GLuint *dlName = NULL;

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
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

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
	    return (state->fontList[inIndex] ? inIndex : 0);
	case GLC_LIST_OBJECT_LIST:
	    if (inIndex > state->listObjectCount) {
		__glcContextState::raiseError(GLC_PARAMETER_ERROR);
		return 0;
	    }
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

GLvoid * glcGetPointer(GLCenum inAttrib)
{
    __glcContextState *state = NULL;

    if (inAttrib != GLC_DATA_POINTER) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return GLC_NONE;
    }

    return state->dataPointer;
}

const GLCchar* glcGetc(GLCenum inAttrib)
{
  static const GLCchar* __glcExtensions = "";
  static const GLCchar* __glcRelease = "draft Linux 2.x";
  static const GLCchar* __glcVendor = "Queso Software";

  __glcContextState *state = NULL;
  __glcUniChar s;
  GLCchar *buffer = NULL;
  int length = 0;

  switch(inAttrib) {
  case GLC_EXTENSIONS:
  case GLC_RELEASE:
  case GLC_VENDOR:
    break;
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

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
    
  length = s.estimate(state->stringType);
  buffer = state->queryBuffer(length);
  if (buffer)
    s.convert(buffer, state->stringType, length);
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return buffer;
}

GLfloat glcGetf(GLCenum inAttrib)
{
    __glcContextState *state = NULL;

    if (inAttrib != GLC_RESOLUTION) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return 0.;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0.;
    }

    return state->resolution;
}

GLfloat* glcGetfv(GLCenum inAttrib, GLfloat* outVec)
{
    __glcContextState *state = NULL;

    if (inAttrib != GLC_BITMAP_MATRIX) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return NULL;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return NULL;
    }

    memcpy(outVec, state->bitmapMatrix, 4 * sizeof(GLfloat));
    
    return outVec;
}

GLint glcGeti(GLCenum inAttrib)
{
    __glcContextState *state = NULL;

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

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

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

GLboolean glcIsEnabled(GLCenum inAttrib)
{
    __glcContextState *state = NULL;

    switch(inAttrib) {
	case GLC_AUTO_FONT:
	case GLC_GL_OBJECTS:
	case GLC_MIPMAP:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return GL_FALSE;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return GL_FALSE;
    }

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

void glcStringType(GLCenum inStringType)
{
    __glcContextState *state = NULL;

    switch(inStringType) {
	case GLC_UCS1:
	case GLC_UCS2:
	case GLC_UCS4:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->stringType = inStringType;
    return;
}
