#include <stdio.h>

#include "ocontext.h"
#include "internal.h"

GLboolean* __glcContextState::isCurrent = NULL;
__glcContextState** __glcContextState::stateList = NULL;
pthread_once_t __glcContextState::initOnce = PTHREAD_ONCE_INIT;
pthread_mutex_t __glcContextState::mutex;
pthread_key_t __glcContextState::contextKey;
pthread_key_t __glcContextState::errorKey;
FT_Library __glcContextState::library;
GDBM_FILE __glcContextState::unidb1 = NULL;
GDBM_FILE __glcContextState::unidb2 = NULL;

__glcContextState::__glcContextState(GLint inContext)
{
  GLint j = 0;

    setState(inContext, this);
    setCurrency(inContext, GL_FALSE);

    catalogList = new __glcStringList(GLC_UCS1);
    if (!catalogList) {
	setState(inContext, NULL);
	raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    id = inContext + 1;
    pendingDelete = GL_FALSE;
    callback = GLC_NONE;
    dataPointer = NULL;
    autoFont = GL_TRUE;
    glObjects = GL_TRUE;
    mipmap = GL_TRUE;
    resolution = 0.;
    bitmapMatrix[0] = 1.;
    bitmapMatrix[1] = 0.;
    bitmapMatrix[2] = 0.;
    bitmapMatrix[3] = 1.;
    currentFontCount = 0;
    fontCount = 0;
    listObjectCount = 0;
    masterCount = 0;
    measuredCharCount = 0;
    renderStyle = GLC_BITMAP;
    replacementCode = 0;
    stringType = GLC_UCS1;
    textureObjectCount = 0;
    versionMajor = 0;
    versionMinor = 2;
    isInCallbackFunc = GL_FALSE;
    
    for (j=0; j < GLC_MAX_CURRENT_FONT; j++)
	currentFontList[j] = 0;

    for (j=0; j < GLC_MAX_FONT; j++)
	fontList[j] = NULL;

    for (j=0; j < GLC_MAX_MASTER; j++)
	masterList[j] = NULL;

    for (j=0; j < GLC_MAX_TEXTURE_OBJECT; j++)
	textureObjectList[j] = 0;
}

__glcContextState::~__glcContextState()
{
  int i = 0;

  delete catalogList;

  for (i = 0; i < GLC_MAX_FONT; i++) {
    __glcFont *font = NULL;
	
    font = fontList[i];
    if (font) {
      glcDeleteFont(i + 1);
    }
  }
    
  for (i = 0; i < GLC_MAX_MASTER; i++) {
    if (masterList[i]) {
      delete masterList[i];
      masterList[i] = NULL;
    }
  }
    
  glcDeleteGLObjects();
}

void __glcContextState::initQueso(void)
{
  int i = 0;

  // Creates the thread-local storage for the GLC error
  // We create it first, so that the error value can be set
  // if something goes wrong afterwards...
  if (pthread_key_create(&errorKey, NULL)) {
    // Unfortunately we have not even been able to allocate a key
    // for thread-local storage. Memory seems to be a really scarse
    // resource here...
    return;
  }

  // Initialize the "Common Area"

  // Create array of state currency
  isCurrent = new GLboolean[GLC_MAX_CONTEXTS];
  if (!isCurrent) {
    raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  // Create array of context states
  stateList = new __glcContextState*[GLC_MAX_CONTEXTS];
  if (!stateList) {
    raiseError(GLC_RESOURCE_ERROR);
    delete[] isCurrent;
    return;
  }

  // Open the first Unicode database
  unidb1 = gdbm_open("database/unicode1.db", 0, GDBM_READER, 0, NULL);
  if (!unidb1) {
    raiseError(GLC_RESOURCE_ERROR);
    delete[] isCurrent;
    delete[] stateList;
    return;
  }

  // Open the second Unicode database
  unidb2 = gdbm_open("database/unicode2.db", 0, GDBM_READER, 0, NULL);
  if (!unidb2) {
    raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(unidb1);
    delete[] isCurrent;
    delete[] stateList;
    return;
  }

  // Initialize the mutex
  // At least this op can not fail !!!
  pthread_mutex_init(&mutex, NULL);

  // Creates the thread-local storage for the context ID
  if (pthread_key_create(&contextKey, NULL)) {
    raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(unidb1);
    gdbm_close(unidb2);
    delete[] isCurrent;
    delete[] stateList;
    return;
  }

  // Initialize FreeType
  if (FT_Init_FreeType(&library)) {
    // Well, if it fails there is nothing that can be done
    // with QuesoGLC : abort...
    raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(unidb1);
    gdbm_close(unidb2);
    delete[] isCurrent;
    delete[] stateList;
    return;
  }
  
  // So far, there are no contexts
  for (i=0; i< GLC_MAX_CONTEXTS; i++) {
    isCurrent[i] = GL_FALSE;
    stateList[i] = NULL;
  }

}

__glcContextState* __glcContextState::getCurrent(void)
{
  return (__glcContextState *)pthread_getspecific(contextKey);
}

__glcContextState* __glcContextState::getState(GLint inContext)
{
  __glcContextState *state = NULL;

  pthread_mutex_lock(&mutex);
  state = stateList[inContext];
  pthread_mutex_unlock(&mutex);

  return state;
}

void __glcContextState::setState(GLint inContext, __glcContextState *inState)
{
  pthread_mutex_lock(&mutex);
  stateList[inContext] = inState;
  pthread_mutex_unlock(&mutex);
}

void __glcContextState::raiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;

  /* An error can only be raised if the current error value is GLC_NONE. 
     However, when inError == GLC_NONE then we must force the current error
     value to GLC_NONE whatever its previous value was */
  error = (GLCenum)pthread_getspecific(errorKey);
  if ((inError && !error) || !inError)
    pthread_setspecific(errorKey, (void *)inError);
}

GLboolean __glcContextState::getCurrency(GLint inContext)
{
  GLboolean current = GL_FALSE;

  pthread_mutex_lock(&mutex);
  current = isCurrent[inContext];
  pthread_mutex_unlock(&mutex);

  return current;
}

void __glcContextState::setCurrency(GLint inContext, GLboolean current)
{
  pthread_mutex_lock(&mutex);
  isCurrent[inContext] = current;
  pthread_mutex_unlock(&mutex);
}

GLboolean __glcContextState::isContext(GLint inContext)
{
  __glcContextState *state = getState(inContext);

  return (state ? GL_TRUE : GL_FALSE);
}

void __glcContextState::addMasters(const GLCchar* inCatalog, GLboolean inAppend)
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
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* Read the # of font files */
    fgets(buffer, 256, file);
    numFontFiles = strtol(buffer, NULL, 10);
    
    for (i = 0; i < numFontFiles; i++) {
	FT_Face face = NULL;
	int numFaces = 0;
	int j = 0;
	char *desc = NULL;
	char *end = NULL;
	char *ext = NULL;
	__glcMaster *master = NULL;
	
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

	/* get the vendor name */
	end = (char *)__glcFindIndexList(desc, 1, "-");
	if (end)
	    end[-1] = 0;

	/* get the extension of the file */
	if (desc)
	    ext = desc - 3;
	else
	    ext = buffer + (strlen(buffer) - 1);
	while ((*ext != '.') && (end - buffer))
	    ext--;
	if (ext != buffer)
	    ext++;
	
	/* open the font file and read it */
	if (!FT_New_Face(__glcContextState::library, path, 0, &face)) {
	    numFaces = face->num_faces;
	    
	    /* If the face has no Unicode charmap, skip it */
	    if (FT_Select_Charmap(face, ft_encoding_unicode))
		continue;
	    
	    for (j = 0; j < masterCount; j++) {
		if (!strcmp(face->family_name, (const char*)masterList[j]->family))
		    break;
	    }
	    
	    if (j < masterCount)
		master = masterList[j];
	    else {

	      if (masterCount < GLC_MAX_MASTER - 1) {
		master = new __glcMaster(face, desc, ext, masterCount, stringType);
		if (!master) {
		    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
		    continue;
		}
		masterList[masterCount++] = master;
	      }
	      else
		__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	    }
	    
	    FT_Done_Face(face);
	}
	else {
	    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
	    continue;
	}

	for (j = 0; j < numFaces; j++) {
	    if (!FT_New_Face(__glcContextState::library, path, j, &face)) {
		if (master->faceList->find(face->style_name))
		    continue;
		else {
		    /* FIXME : 
		       If there are several faces into the same file then we
		       should indicate it inside faceFileName
		     */
		    if (inAppend) {
			if (master->faceList->append(face->style_name))
			    break;
			if (master->faceFileName->append(path)) {
			    master->faceList->remove(face->style_name);
			    break;		    
			}
		    }
		    else {
			if (master->faceList->prepend(face->style_name))
			    break;
			if (master->faceFileName->prepend(path)) {
			    master->faceList->remove(face->style_name);
			    break;		    
			}
		    }
		}
		
	    }
	    else {
		__glcContextState::raiseError(GLC_RESOURCE_ERROR);
		continue;
	    }
	    FT_Done_Face(face);
	}
    }
    
    if (inAppend) {
      if (catalogList->append(inCatalog)) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }
    else {
      if (catalogList->prepend(inCatalog)) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }

    fclose(file);
}

void __glcContextState::removeMasters(GLint inIndex)
{
    const char* fileName = "/fonts.dir";
    char buffer[256];
    char path[256];
    int numFontFiles = 0;
    int i = 0;
    FILE *file;

    /* TODO : use Unicode instead of ASCII */
    strncpy(buffer, (const char*)catalogList->extract(inIndex, buffer, 256), 256);
    strncpy(path, buffer, 256);
    strncat(path, fileName, strlen(fileName));
    
    /* Open 'fonts.dir' */
    file = fopen(path, "r");
    if (!file) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* Read the # of font files */
    fgets(buffer, 256, file);
    numFontFiles = strtol(buffer, NULL, 10);
    
    for (i = 0; i < numFontFiles; i++) {
	int j = 0;
	char *desc = NULL;
	GLint index = 0;
	
	/* get the file name */
	fgets(buffer, 256, file);
	desc = (char *)__glcFindIndexList(buffer, 1, " ");
	if (desc) {
	    desc[-1] = 0;
	    desc++;
	}

	for (j = 0; j < GLC_MAX_MASTER; j++) {
	  __glcMaster *master = masterList[j];
	    if (!master)
		continue;
	    index = master->faceFileName->getIndex(buffer);
	    if (index) {
		master->faceFileName->removeIndex(index);
		master->faceList->removeIndex(index);
		/* Characters from the font should be removed from the char list */
	    }
	    if (!master->faceFileName->getCount()) {

	      for (i = 0; i < GLC_MAX_FONT; i++) {
		if (fontList[i]) {
		  if (fontList[i]->parent == master)
		    glcDeleteFont(i + 1);
		}
	      }

	      delete master;
	      master = NULL;
	      masterCount--;
	    }
	}
    }
    
    fclose(file);
    return;
}

static GLint __glcLookupFont(GLint inCode, __glcContextState *inState)
{
    GLint i = 0;
    
    for (i = 0; i < inState->currentFontCount; i++) {
	const GLint font = inState->currentFontList[i];
	if (glcGetFontMap(font, inCode))
	    return font;
    }
    return 0;
}

static GLboolean __glcCallCallbackFunc(GLint inCode, __glcContextState *inState)
{
    GLCfunc callbackFunc = NULL;
    GLboolean result = GL_FALSE;
    
    callbackFunc = inState->callback;
    if (!callbackFunc)
	return GL_FALSE;

    inState->isInCallbackFunc = GL_TRUE;
    result = (*callbackFunc)(inCode);
    inState->isInCallbackFunc = GL_FALSE;
    
    return result;
}

GLint __glcContextState::getFont(GLint inCode)
{
    GLint font = 0;
    
    font = __glcLookupFont(inCode, this);
    if (font)
	return font;

    if (__glcCallCallbackFunc(inCode, this)) {
	font = __glcLookupFont(inCode, this);
	if (font)
	    return font;
    }

    if (autoFont) {
	GLint i = 0;
	
	for (i = 0; i < fontCount; i++) {
	    font = glcGetListi(GLC_FONT_LIST, i);
	    if (glcGetFontMap(font, inCode)) {
		glcAppendFont(font);
		return font;
	    }
	}
	
	for (i = 0; i < masterCount; i++) {
	    if (glcGetMasterMap(i, inCode)) {
		font = glcNewFontFromMaster(glcGenFontID(), i);
		if (font) {
		    glcAppendFont(font);
		    return font;
		}
	    }
	}
    }
    return 0;
}
