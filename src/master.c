/* $Id$ */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "GL/glc.h"
#include "internal.h"

const GLCchar* glcGetMasterListc(GLint inMaster, GLCenum inAttrib, GLint inIndex)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar* s = NULL;
  GLCchar *buffer = NULL;
  int length = 0;
    
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  if ((inMaster < 0) || (inMaster >= state->masterCount)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
    
  master = state->masterList[inMaster];
    
  switch(inAttrib) {
  case GLC_CHAR_LIST:
    if ((inIndex < 0) || (inIndex >= master->charListCount)) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    else {
      /* TODO : return the right thing */
      return GLC_NONE;
    }
  case GLC_FACE_LIST:
    if ((inIndex < 0) || (inIndex >= master->faceList->getCount())) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      return GLC_NONE;
    }
    else {
      s = master->faceList->findIndex(inIndex);
      break;
    }
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  length = s->estimate(state->stringType);
  buffer = state->queryBuffer(length);
  if (buffer)
    s->convert(buffer, state->stringType, length);
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
  return buffer;
}

/* NYI */
const GLCchar* glcGetMasterMap(GLint inMaster, GLint inCode)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar *s = NULL;
  GLCchar *buffer = NULL;
  int length = 0;
  FT_Face face = NULL;
  FT_UInt glyphIndex = 0;
  datum key, content;
  GLint i = 0;

  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  if ((inMaster < 0) || (inMaster >= state->masterCount)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
    
  master = state->masterList[inMaster];

  for (i = 0; i < master->faceFileName->getCount(); i++) {
    s = master->faceFileName->findIndex(i);
    buffer = state->queryBuffer(s->lenBytes());
    if (!buffer) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    s->dup(buffer, s->lenBytes());

    if (FT_New_Face(state->library, 
		    (const char*) buffer, 0, &face)) {
      /* Unable to load the face file, however this should not happen since
	 it has been succesfully loaded when the master was created */
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      face = NULL;
      return GLC_NONE;
    }
    glyphIndex = FT_Get_Char_Index(face, inCode);
    if (glyphIndex)
      break;
    if (face) {
      FT_Done_Face(face);
      face = NULL;
    }
  }
    
  if (i == master->faceFileName->getCount())
    if (face) {
      FT_Done_Face(face);
      face = NULL;
    }
    
  key.dsize = sizeof(GLint);
  key.dptr = (char *)&inCode;
  content = gdbm_fetch(__glcContextState::unidb1, key);
  if (!content.dptr) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  s = new __glcUniChar(content.dptr, GLC_UCS1);
  length = s->estimate(state->stringType);
  buffer = state->queryBuffer(length);
  if (!buffer) {
    delete s;
    free(content.dptr);
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  s->convert(buffer, state->stringType, length);
  delete s;
  free(content.dptr);
    
  return buffer;
}

const GLCchar* glcGetMasterc(GLint inMaster, GLCenum inAttrib)
{
  __glcContextState *state = NULL;
  __glcMaster *master = NULL;
  __glcUniChar *s = NULL;
  GLCchar *buffer = NULL;
  int length = 0;

  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return GLC_NONE;
  }

  if ((inMaster < 0) || (inMaster >= state->masterCount)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
  
  master = state->masterList[inMaster];
  
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
  default:
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
  
  length = s->estimate(state->stringType);
  buffer = state->queryBuffer(length);
  if (buffer)
    s->convert(buffer, state->stringType, length);
  else
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);

  return buffer;  
}

GLint glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
    __glcContextState *state = NULL;
    __glcMaster *master = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return 0;
    }

    if ((inMaster < 0) || (inMaster >= state->masterCount)) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    master = state->masterList[inMaster];
    
    switch(inAttrib) {
        case GLC_CHAR_COUNT:
	    return master->charListCount;
	case GLC_FACE_COUNT:
	    return master->faceList->getCount();
	case GLC_IS_FIXED_PITCH:
	    return master->isFixedPitch;
	case GLC_MAX_MAPPED_CODE:
	    return master->maxMappedCode;
	case GLC_MIN_MAPPED_CODE:
	    return master->minMappedCode;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return GLC_NONE;
    }
    
    return 0;
}

void glcAppendCatalog(const GLCchar* inCatalog)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->addMasters(inCatalog, GL_TRUE);
}

void glcPrependCatalog(const GLCchar* inCatalog)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->addMasters(inCatalog, GL_FALSE);
}

void glcRemoveCatalog(GLint inIndex)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    if ((inIndex < 0) || (inIndex >= state->catalogList->getCount())) {
	__glcContextState::raiseError(GLC_PARAMETER_ERROR);
	return;
    }
    
    state->removeMasters(inIndex);
}
