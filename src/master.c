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
    GLCchar* s = GLC_NONE;
    
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

    return s;
}

/* NYI */
const GLCchar* glcGetMasterMap(GLint inMaster, GLint inCode)
{
    __glcContextState *state = NULL;
    __glcMaster *master = NULL;
    FT_Face face;
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
	if (FT_New_Face(__glcContextState::library, 
			(const char*)master->faceFileName->findIndex(i),
			0, &face)) {
	    /* Unable to load the face file, however this should not happen since
	       it has been succesfully loaded when the master was created */
	    __glcContextState::raiseError(GLC_INTERNAL_ERROR);
	    return GLC_NONE;
	}
	glyphIndex = FT_Get_Char_Index(face, inCode);
	if (glyphIndex)
	    break;
	FT_Done_Face(face);
    }
    
    if (i == master->faceFileName->getCount())
	FT_Done_Face(face);
    
    key.dsize = sizeof(GLint);
    key.dptr = (char *)&inCode;
    content = gdbm_fetch(__glcContextState::unidb1, key);
    if (!content.dptr) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return GLC_NONE;
    }
    strncpy(state->__glcBuffer, content.dptr, 256);
    free(content.dptr);
    
    return (const GLCchar*)state->__glcBuffer;
}

const GLCchar* glcGetMasterc(GLint inMaster, GLCenum inAttrib)
{
    __glcContextState *state = NULL;
    __glcMaster *master = NULL;

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
	    return master->family;
	case GLC_MASTER_FORMAT:
	    return master->masterFormat;
	case GLC_VENDOR:
	    return master->vendor;
	case GLC_VERSION:
	    return master->version;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return GLC_NONE;
    }
    
    return GLC_NONE;
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
