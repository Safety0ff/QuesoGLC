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
    
    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return GLC_NONE;
    }

    if ((inMaster < 0) || (inMaster >= state->masterCount)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    master = state->masterList[inMaster];
    
    switch(inAttrib) {
	case GLC_CHAR_LIST:
	    if ((inIndex < 0) || (inIndex >= master->charListCount)) {
		__glcRaiseError(GLC_PARAMETER_ERROR);
		return GLC_NONE;
	    }
	    else {
		/* TODO : return the right thing */
		return GLC_NONE;
	    }
	case GLC_FACE_LIST:
	    if ((inIndex < 0) || (inIndex >= master->faceList.count)) {
		__glcRaiseError(GLC_PARAMETER_ERROR);
		return GLC_NONE;
	    }
	    else {
		s = __glcStringListExtractElement(&master->faceList, inIndex, (GLCchar *) __glcBuffer, GLC_STRING_CHUNK);
		break;
	    }
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
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
    char buffer[256];

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return GLC_NONE;
    }

    if ((inMaster < 0) || (inMaster >= state->masterCount)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    master = state->masterList[inMaster];

    for (i = 0; i < master->faceFileName.count; i++) {
	if (FT_New_Face(library, __glcStringListExtractElement(&master->faceFileName,
		i, buffer, 256), 0, &face)) {
	    /* Unable to load the face file, however this should not happen since
	       it has been succesfully loaded when the master was created */
	    __glcRaiseError(GLC_INTERNAL_ERROR);
	    return GLC_NONE;
	}
	glyphIndex = FT_Get_Char_Index(face, inCode);
	if (glyphIndex)
	    break;
	FT_Done_Face(face);
    }
    
    if (i == master->faceFileName.count)
	FT_Done_Face(face);
    
    key.dsize = sizeof(GLint);
    key.dptr = (char *)&inCode;
    content = gdbm_fetch(unicod1, key);
    if (!content.dptr) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return GLC_NONE;
    }
    strncpy(__glcBuffer, content.dptr, 256);
    free(content.dptr);
    
    return (const GLCchar*)__glcBuffer;
}

const GLCchar* glcGetMasterc(GLint inMaster, GLCenum inAttrib)
{
    __glcContextState *state = NULL;
    __glcMaster *master = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return GLC_NONE;
    }

    if ((inMaster < 0) || (inMaster >= state->masterCount)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
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
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return GLC_NONE;
    }
    
    return GLC_NONE;
}

GLint glcGetMasteri(GLint inMaster, GLCenum inAttrib)
{
    __glcContextState *state = NULL;
    __glcMaster *master = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return 0;
    }

    if ((inMaster < 0) || (inMaster >= state->masterCount)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return GLC_NONE;
    }
    
    master = state->masterList[inMaster];
    
    switch(inAttrib) {
        case GLC_CHAR_COUNT:
	    return master->charListCount;
	case GLC_FACE_COUNT:
	    return master->faceList.count;
	case GLC_IS_FIXED_PITCH:
	    return master->isFixedPitch;
	case GLC_MAX_MAPPED_CODE:
	    return master->maxMappedCode;
	case GLC_MIN_MAPPED_CODE:
	    return master->minMappedCode;
	default:
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return GLC_NONE;
    }
    
    return 0;
}

static __glcMaster* __glcCreateMaster(FT_Face face, __glcContextState* inState)
{
    /* FIXME : if a master has been deleted by glcRemoveCatalog then its location
     * may be free and should be used instead of using the last location
     */
    __glcMaster *master = NULL;
    
    if (inState->masterCount < GLC_MAX_MASTER - 1) {

	master = (__glcMaster*)malloc(sizeof(__glcMaster));
	if (!master)
	    return NULL;

	master->family = (GLCchar *)malloc(strlen(face->family_name) + 1);
	if (!master->family) {
	    free(master);
	    return NULL;
	}
	memcpy(master->family, face->family_name, strlen(face->family_name) + 1);

	if (__glcStringListInit(&master->faceList, inState)) {
	    free(master->family);
	    free(master);
	    return NULL;
	}
	if (__glcStringListInit(&master->faceFileName, inState)) {
	    __glcStringListDelete(&master->faceList);
	    free(master->family);
	    free(master);
	    return NULL;
	}

	master->charListCount = 0;
	master->masterFormat = NULL;
	master->vendor = NULL;
	master->version = NULL;
	master->isFixedPitch = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? GL_TRUE : GL_FALSE;
	master->minMappedCode = 0;
	master->maxMappedCode = 0x7fffffff;
	inState->masterList[inState->masterCount] = master;
	master->id = inState->masterCount;
	inState->masterCount++;
	
	return master;
    }
    else
	return NULL;
}

/* NYI */
static int __glcUpdateMasters(const GLCchar* inCatalog, __glcContextState *inState, GLboolean inAppend)
{
    const char* fileName = "/fonts.dir";
    char *s = (char *)inCatalog;
    char buffer[256];
    char path[256];
    int numFontFiles = 0;
    int i = 0;
    FILE *file;

    /* TODO : use Unicode instead of ASCII */
    strncpy(path, s, 256);
    strncat(path, fileName, strlen(fileName));
    
    /* Open 'fonts.dir' */
    file = fopen(path, "r");
    if (!file) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return 1;
    }
    
    /* Read the # of font files */
    fgets(buffer, 256, file);
    numFontFiles = strtol(buffer, NULL, 10);
    
    for (i = 0; i < numFontFiles; i++) {
	FT_Face face = NULL;
	int numFaces = 0;
	int j = 0;
	char *desc = NULL;
	__glcMaster *master;
	
	/* get the file name */
	fgets(buffer, 256, file);
	desc = (char *)__glcFindIndexList(buffer, 1, " ");
	if (desc) {
	    desc[-1] = 0;
	    desc++;
	}
	strncpy(path, s, 256);
	strncat(path, "/", 1);
	strncat(path, buffer, strlen(buffer));

	/* open the font file and read it */
	if (!FT_New_Face(library, path, 0, &face)) {
	    numFaces = face->num_faces;
	    
	    /* If the face has no Unicode charmap, skip it */
	    if (FT_Select_Charmap(face, ft_encoding_unicode))
		continue;
	    
	    for (j = 0; j < inState->masterCount; j++) {
		if (!strcmp(face->family_name, inState->masterList[j]->family))
		    break;
	    }
	    
	    if (j < inState->masterCount)
		master = inState->masterList[j];
	    else {
		master = __glcCreateMaster(face, inState);
		if (!master) {
		    FT_Done_Face(face);
		    __glcRaiseError(GLC_RESOURCE_ERROR);
		    continue;
		}
	    }
	    
	    FT_Done_Face(face);
	}
	else {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    continue;
	}

	for (j = 0; j < numFaces; j++) {
	    if (!FT_New_Face(library, path, j, &face)) {
		if (__glcStringListFind(&master->faceList, face->style_name))
		    continue;
		else {
		    /* FIXME : 
		       If there are several faces into the same file then we
		       should indicate it inside faceFileName
		     */
		    if (inAppend) {
			if (__glcStringListAppend(&master->faceList, face->style_name))
			    break;
			if (__glcStringListAppend(&master->faceFileName, path)) {
			    __glcStringListRemove(&master->faceList, face->style_name);
			    break;		    
			}
		    }
		    else {
			if (__glcStringListPrepend(&master->faceList, face->style_name))
			    break;
			if (__glcStringListPrepend(&master->faceFileName, path)) {
			    __glcStringListRemove(&master->faceList, face->style_name);
			    break;		    
			}
		    }
		}
		
	    }
	    else {
		__glcRaiseError(GLC_RESOURCE_ERROR);
		continue;
	    }
	    FT_Done_Face(face);
	}
    }
    
    fclose(file);
    return 0;
}

void glcAppendCatalog(const GLCchar* inCatalog)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    if (!__glcUpdateMasters(inCatalog, state, GL_TRUE)) {
	if (__glcStringListAppend(&state->catalogList, inCatalog)) {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    return;
	}
    }
}

void glcPrependCatalog(const GLCchar* inCatalog)
{
    __glcContextState *state = NULL;

    state = __glcGetCurrentState();
    if (!state) {
	__glcRaiseError(GLC_STATE_ERROR);
	return;
    }

    if (!__glcUpdateMasters(inCatalog, state, GL_FALSE)) {
	if (__glcStringListPrepend(&state->catalogList, inCatalog)) {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    return;
	}
    }
}

/* NYI */
void glcRemoveCatalog(GLint inIndex)
{
}
