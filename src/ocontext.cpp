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

    for (j=0; j < GLC_MAX_LIST_OBJECT; j++)
	listObjectList[j] = 0;

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
    if (masterList[i])
      __glcDeleteMaster(i, this);
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
