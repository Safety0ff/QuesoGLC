#include <stdio.h>

#include "ocontext.h"
#include "internal.h"

GLboolean* __glcContextState::isCurrent = NULL;
__glcContextState** __glcContextState::stateList = NULL;
pthread_mutex_t __glcContextState::mutex;
pthread_key_t __glcContextState::contextKey;
pthread_key_t __glcContextState::errorKey;
pthread_key_t __glcContextState::lockKey;
FT_Library __glcContextState::library;
GDBM_FILE __glcContextState::unidb1 = NULL;
GDBM_FILE __glcContextState::unidb2 = NULL;

__glcContextState::__glcContextState(GLint inContext)
{
  GLint j = 0;

  setState(inContext, this);
  setCurrency(inContext, GL_FALSE);

  catalogList = new __glcStringList(NULL);
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

  buffer = NULL;
  bufferSize = 0;
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

  if (bufferSize)
    free(buffer);

  glcDeleteGLObjects();
}

/* Get the current context of the issuing thread */
__glcContextState* __glcContextState::getCurrent(void)
{
  return (__glcContextState *)pthread_getspecific(contextKey);
}

/* Get the context state of a given context */
__glcContextState* __glcContextState::getState(GLint inContext)
{
  __glcContextState *state = NULL;

  lock();
  state = stateList[inContext];
  unlock();

  return state;
}

/* Set the context state of a given context */
void __glcContextState::setState(GLint inContext, __glcContextState *inState)
{
  lock();
  stateList[inContext] = inState;
  unlock();
}

/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcContextState::raiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;

  /* An error can only be raised if the current error value is GLC_NONE. 
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = (GLCenum)pthread_getspecific(errorKey);
  if ((inError && !error) || !inError)
    pthread_setspecific(errorKey, (void *)inError);
}

/* Determine whether a context is current to a thread or not */
GLboolean __glcContextState::getCurrency(GLint inContext)
{
  GLboolean current = GL_FALSE;

  lock();
  current = isCurrent[inContext];
  unlock();

  return current;
}

/* Make a context current or release it. This value is intended to determine
 * if a context is current to a thread or not. Array isCurrent[] exists
 * because there is no simple way to get the contextKey value of
 * other threads (we do not even know how many threads have been created)
 */
void __glcContextState::setCurrency(GLint inContext, GLboolean current)
{
  lock();
  isCurrent[inContext] = current;
  unlock();
}

/* Determine if a context ID is associated to a context state or not. This is
 * a different notion than the currency of a context : a context state may
 * exist without being associated to a thread.
 */
GLboolean __glcContextState::isContext(GLint inContext)
{
  __glcContextState *state = getState(inContext);

  return (state ? GL_TRUE : GL_FALSE);
}

/* This function is called by glcAppendCatalog and glcPrependCatalog. It has
 * been associated to the contextState class rather than the master class
 * because glcAppendCatalog and glcPrependCatalog might create several masters.
 * Moreover it associates the new masters (if any) to the current context.
 */
void __glcContextState::addMasters(const GLCchar* inCatalog, GLboolean inAppend)
{
  const char* fileName = "/fonts.dir";
  char buffer[256];
  char path[256];
  int numFontFiles = 0;
  int i = 0;
  FILE *file;
  __glcUniChar s;

  /* TODO : use Unicode instead of ASCII ? */
  strncpy(path, (const char*)inCatalog, 256);
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
    strncpy(path, (const char*)inCatalog, 256);
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

      /* Determine if the family (i.e. "Times", "Courier", ...) is already
       * associated to a master.
       */
      for (j = 0; j < masterCount; j++) {
	s = __glcUniChar(face->family_name, GLC_UCS1);
	if (!s.compare(masterList[j]->family))
	  break;
      }

      if (j < masterCount)
	/* We have found the master corresponding to the family of the current
	 * font file.
	 */
	master = masterList[j];
      else {
	/* No master has been found. We must create one */

	if (masterCount < GLC_MAX_MASTER - 1) {
	  /* Create a new master and add it to the current context */
	  master = new __glcMaster(face, desc, ext, masterCount, stringType);
	  if (!master) {
	    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
	    continue;
	  }
	  /* FIXME : use the first free location instead of this one */
	  masterList[masterCount++] = master;
	}
	else {
	  /* We have already created the maximum number of masters allowed */
	  __glcContextState::raiseError(GLC_RESOURCE_ERROR);
	  continue;
	}
      }

      FT_Done_Face(face);
    }
    else {
      /* FreeType is not able to open the font file, try the next one */
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      continue;
    }

    /* For each face in the font file */
    for (j = 0; j < numFaces; j++) {
      __glcUniChar sp = __glcUniChar(path, GLC_UCS1);
      if (!FT_New_Face(__glcContextState::library, path, j, &face)) {
	s = __glcUniChar(face->style_name, GLC_UCS1);
	if (master->faceList->find(&s))
	  /* The current face in the font file has already been loaded in a
	   * master, try the next one
	   */
	  continue;
	else {
	  /* FIXME : 
	     If there are several faces into the same file then we
	     should indicate it inside faceFileName
	   */
	  /* Append (or prepend) the new face and its file name to the master */
	  if (inAppend) {
	    if (master->faceList->append(&s))
	      break;
	    if (master->faceFileName->append(&sp)) {
	      master->faceList->remove(&s);
	      break;		    
	    }
	  }
	  else {
	    if (master->faceList->prepend(&s))
	      break;
	    if (master->faceFileName->prepend(&sp)) {
	      master->faceList->remove(&s);
	      break;		    
	    }
	  }
	}

      }
      else {
      /* FreeType is not able to read this face in the font file, 
       * try the next one.
       */
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	continue;
      }
      FT_Done_Face(face);
    }
  }

  /* Append (or prepend) the directory name to the catalog list */
  s = __glcUniChar(inCatalog, GLC_UCS1);
  if (inAppend) {
    if (catalogList->append(&s)) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }
  else {
    if (catalogList->prepend(&s)) {
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }

  /* Closes 'fonts.dir' */
  fclose(file);
}

/* This function is called by glcRemoveCatalog. It has been included in the
 * context state class for the same reasons than addMasters was.
 */
void __glcContextState::removeMasters(GLint inIndex)
{
  const char* fileName = "/fonts.dir";
  char buffer[256];
  char path[256];
  int numFontFiles = 0;
  int i = 0;
  FILE *file;
  __glcUniChar s;

  /* TODO : use Unicode instead of ASCII */
  strncpy(buffer, (const char*)catalogList->findIndex(inIndex), 256);
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

    /* Search the file name of the font in the masters */
    for (j = 0; j < GLC_MAX_MASTER; j++) {
      __glcMaster *master = masterList[j];
      if (!master)
	continue;
      s = __glcUniChar(buffer, GLC_UCS1);
      index = master->faceFileName->getIndex(&s);
      if (!index)
	continue; // The file is not in the current master, try the next one

      /* Removes the corresponding faces in the font list */
      for (i = 0; i < GLC_MAX_FONT; i++) {
	__glcFont *font = fontList[i];
	if (font) {
	  if ((font->parent == master) && (font->faceID == index)) {
	    /* Eventually remove the font from the current font list */
	    for (j = 0; j < GLC_MAX_CURRENT_FONT; j++) {
	      if (currentFontList[j] == i) {
		memmove(&currentFontList[j], &currentFontList[j + 1], currentFontCount - j - 1);
		currentFontCount--;
		break;
	      }
	    }
	    glcDeleteFont(i + 1);
	  }
	}
      }

      master->faceFileName->removeIndex(index); // Remove the file name
      master->faceList->removeIndex(index); // Remove the face

      /* FIXME :Characters from the font should be removed from the char list */

      /* If the master is empty (i.e. does not contain any face) then
       * remove it.
       */
      if (!master->faceFileName->getCount()) {
	delete master;
	master = NULL;
	masterCount--;
      }
    }
  }

  fclose(file);
  return;
}

/* Return the ID of the first font in GLC_CURRENT_FONT_LIST the maps 'inCode'
 * If there is no such font, the function returns zero
 */
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

/* Calls the callback function (does various tests to determine if it is
 * possible) and returns GL_TRUE if it has succeeded or GL_FALSE otherwise.
 */
static GLboolean __glcCallCallbackFunc(GLint inCode, __glcContextState *inState)
{
  GLCfunc callbackFunc = NULL;
  GLboolean result = GL_FALSE;

  /* Recursivity is not allowed */
  if (inState->isInCallbackFunc)
    return GL_FALSE;

  callbackFunc = inState->callback;
  if (!callbackFunc)
    return GL_FALSE;

  inState->isInCallbackFunc = GL_TRUE;
  result = (*callbackFunc)(inCode);
  inState->isInCallbackFunc = GL_FALSE;

  return result;
}

/* Returns the ID of the first font in GLC_CURRENT_FONT_LIST that maps
 * 'inCode'. If there is no such font, the function attempts to append a new
 * font from GLC_FONT_LIST (or from a master) to GLC_CURRENT_FONT_LIST. If the
 * attempt fails the function returns zero.
 */
GLint __glcContextState::getFont(GLint inCode)
{
  GLint font = 0;

  font = __glcLookupFont(inCode, this);
  if (font)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(inCode, this)) {
    font = __glcLookupFont(inCode, this);
    if (font)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (autoFont) {
    GLint i = 0;

    for (i = 0; i < fontCount; i++) {
      font = glcGetListi(GLC_FONT_LIST, i);
      if (glcGetFontMap(font, inCode)) {
	glcAppendFont(font);
	return font;
      }
    }

    /* Otherwise, the function searches the GLC master list for the first
     * master that maps 'inCode'. If the search succeeds, it creates a font
     * from the master and appends its ID to GLC_CURRENT_FONT_LIST.
     */
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

void __glcContextState::lock(void)
{
  int lockCount = (int)pthread_getspecific(lockKey);

  if (!lockCount)
    pthread_mutex_lock(&mutex);

  lockCount++;
  pthread_setspecific(lockKey, (void*)lockCount);
}

void __glcContextState::unlock(void)
{
  int lockCount = (int)pthread_getspecific(lockKey);

  lockCount--;
  if (!lockCount)
    pthread_mutex_unlock(&mutex);

  pthread_setspecific(lockKey, (void*)lockCount);
}

GLCchar* __glcContextState::queryBuffer(int inSize)
{
  if (inSize > bufferSize) {
    buffer = (GLCchar*)realloc(buffer, inSize);
    if (!buffer)
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    else
      bufferSize = inSize;
  }

  return buffer;
}
