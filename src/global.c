/** \file global.c
  * \brief Global Commands
  */
/* $Id$ */
#include <stdlib.h>
#include <stdio.h>

#include "GL/glc.h"
#include "ocontext.h"
#include "internal.h"

/* This function is supposed to be called when QuesoGLC is no longer needed.
 * It frees the memory and closes the Unicode DB files
 */
void __glcExitLibrary(void)
{
  int i;

  /* destroy remaining contexts */
  for (i=0; i < GLC_MAX_CONTEXTS; i++) {
    if (__glcContextState::isContext(i))
      delete __glcContextState::getState(i);
  }

  /* destroy Common Area */
  delete[] __glcContextState::isCurrent;
  delete[] __glcContextState::stateList;

  gdbm_close(__glcContextState::unidb1);
  gdbm_close(__glcContextState::unidb2);

  pthread_mutex_destroy(&__glcContextState::mutex);
  pthread_key_delete(__glcContextState::contextKey);
  pthread_key_delete(__glcContextState::errorKey);
  pthread_key_delete(__glcContextState::lockKey);

  FT_Done_FreeType(__glcContextState::library);
}

/* This function is supposed to be called before any of QuesoGLC
 * function is used. It reserves memory and opens the Unicode DB files.
 */
void __glcInitLibrary(void)
{
  int i = 0;

  // Creates the thread-local storage for the GLC error
  // We create it first, so that the error value can be set
  // if something goes wrong afterwards...
  if (pthread_key_create(&__glcContextState::errorKey, NULL)) {
    // Unfortunately we have not even been able to allocate a key
    // for thread-local storage. Memory seems to be a really scarse
    // resource here...
    return;
  }

  // Initialize the "Common Area"

  // Create array of state currency
  __glcContextState::isCurrent = new GLboolean[GLC_MAX_CONTEXTS];
  if (!__glcContextState::isCurrent) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }

  // Create array of context states
  __glcContextState::stateList = new __glcContextState*[GLC_MAX_CONTEXTS];
  if (!__glcContextState::stateList) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    delete[] __glcContextState::isCurrent;
    return;
  }

  // Open the first Unicode database
  __glcContextState::unidb1 = gdbm_open("database/unicode1.db", 0, GDBM_READER, 0, NULL);
  if (!__glcContextState::unidb1) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    return;
  }

  // Open the second Unicode database
  __glcContextState::unidb2 = gdbm_open("database/unicode2.db", 0, GDBM_READER, 0, NULL);
  if (!__glcContextState::unidb2) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(__glcContextState::unidb1);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    return;
  }

  // Initialize the mutex
  // At least this op can not fail !!!
  pthread_mutex_init(&__glcContextState::mutex, NULL);

  // Creates the thread-local storage for the context ID
  if (pthread_key_create(&__glcContextState::contextKey, NULL)) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(__glcContextState::unidb1);
    gdbm_close(__glcContextState::unidb2);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    return;
  }

  // Creates the thread-local storage for the lock count
  if (pthread_key_create(&__glcContextState::lockKey, NULL)) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(__glcContextState::unidb1);
    gdbm_close(__glcContextState::unidb2);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    return;
  }

  // Initialize FreeType
  if (FT_Init_FreeType(&__glcContextState::library)) {
    // Well, if it fails there is nothing that can be done
    // with QuesoGLC : abort...
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(__glcContextState::unidb1);
    gdbm_close(__glcContextState::unidb2);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    return;
  }

  // So far, there are no contexts
  for (i=0; i< GLC_MAX_CONTEXTS; i++) {
    __glcContextState::isCurrent[i] = GL_FALSE;
    __glcContextState::stateList[i] = NULL;
  }

  atexit(__glcExitLibrary);
}

/** \ingroup global
  * Returns <b>GL_TRUE</b> if <i>inContext</i> is the ID of one of the client's
  * GLC contexts.
  */
GLboolean glcIsContext(GLint inContext)
{
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS))
    return GL_FALSE;
  else
    return __glcContextState::isContext(inContext - 1);
}

/** \ingroup global
  * Returns the value of the issueing thread's current GLC context ID variable
  */
GLint glcGetCurrentContext(void)
{
  __glcContextState *state = NULL;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  state = __glcContextState::getCurrent();
  if (!state)
    return 0;
  else
    return state->id;
}

/** \ingroup global
  * Marks for deletion the GLC context identified by <i>inContext</i>. If the
  * marked context is not current to any client thread, the command deletes
  * the marked context immediatly. Otherwise, the marked context will be deleted
  * during the execution of the next glcContext() command that causes it not to
  * be current to any client thread. The command raises <b>GLC_PARAMETER_ERROR</b>
  * if <i>inContext</i> is not the ID of one of the client's GLC contexts.
  */
void glcDeleteContext(GLint inContext)
{
  __glcContextState *state = NULL;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  /* verify parameters are in legal bounds */
  if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  state = __glcContextState::getState(inContext - 1);

  /* verify that the context has been created */
  if (!state) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  __glcContextState::lock();

  if (__glcContextState::getCurrency(inContext - 1))
    /* The context is current to a thread : just mark for deletion */
    state->pendingDelete = GL_TRUE;
  else
    delete state;

  __glcContextState::unlock();
}

/** \ingroup global
  * Assigns the value <i>inContext</i> to the issuing thread's current GLC
  * context ID variable. The command raises <b>GLC_PARAMETER_ERROR</b> if
  * <i>inContext</i> is not zero and is not the ID of one of the client's GLC
  * contexts. The command raises <b>GLC_STATE_ERROR</b> if <i>inContext</i> is
  * the ID of a GLC context that is current to a thread other than the issuing
  * thread. The command raises <b>GLC_STATE_ERROR</b> if the issuing thread is
  * executing a callback function that has been called from GLC.
  */
void glcContext(GLint inContext)
{
  char *version = NULL;
  char *extension = NULL;
  __glcContextState *state = NULL;
  GLint currentContext = 0;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  __glcContextState::lock();

  if (inContext) {
    /* verify that parameters are in legal bounds */
    if ((inContext < 0) || (inContext > GLC_MAX_CONTEXTS)) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      __glcContextState::unlock();
      return;
    }
    /* verify that the context exists */
    if (!__glcContextState::isContext(inContext - 1)) {
      __glcContextState::raiseError(GLC_PARAMETER_ERROR);
      __glcContextState::unlock();
      return;
    }

    /* Gets the current context ID */
    currentContext = glcGetCurrentContext();

    /* Check if the issuing thread is executing a callback
     * function that has been called from GLC
     */
    if (currentContext) {
      state = __glcContextState::getCurrent();
      if (state->isInCallbackFunc) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
      __glcContextState::unlock();
	return;
      }
    }

    /* Is the context already current to a thread ? */
    if (__glcContextState::getCurrency(inContext - 1)) {
      /* If the context is current to another thread -> ERROR ! */
      if (currentContext != inContext)
	__glcContextState::raiseError(GLC_STATE_ERROR);

      /* If we get there, this means that the context 'inContext'
       * is already current to one thread (whether it is the issueing thread
       * or not) : there is nothing else to be done.
       */
      __glcContextState::unlock();
      return;
    }

    /* Release old current context if any */
    if (currentContext) {
      __glcContextState::setCurrency(currentContext - 1, GL_FALSE);
      state = __glcContextState::getState(currentContext - 1);
    }

    /* Make the context current to the thread */
    pthread_setspecific(__glcContextState::contextKey, (__glcContextState::stateList)[inContext - 1]);
    __glcContextState::setCurrency(inContext - 1, GL_TRUE);
  }
  else {
    /* inContext is null, the current thread must release its context if any */
    GLint currentContext = 0;

    /* Gets the current context ID */
    currentContext = glcGetCurrentContext();

    if (currentContext) {
      state = __glcContextState::getState(currentContext - 1);
      /* Deassociate the context from the issuing thread */
      pthread_setspecific(__glcContextState::contextKey, NULL);
      /* Release the context */
      __glcContextState::setCurrency(currentContext - 1, GL_FALSE);
    }
  }

  /* execute pending deletion if any */
  if (currentContext && state) {
    if (state->pendingDelete)
      delete state;
  }

  __glcContextState::unlock();

  /* We read the version and extensions of the OpenGL client. We do it
   * for compliance with the specifications because we do not use it.
   * However it may be useful if QuesoGLC tries to use some GL commands 
   * that are not part of OpenGL 1.0
   */
  version = (char *)glGetString(GL_VERSION);
  extension = (char *)glGetString(GL_EXTENSIONS);
}

/** \ingroup global
  * Generates a new GLC context and returns its ID.
  */
GLint glcGenContext(void)
{
  int i = 0;
  __glcContextState *state = NULL;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  __glcContextState::lock();

  /* Search for the first context ID that is unused */
  for (i=0 ; i<GLC_MAX_CONTEXTS; i++) {
    if (!__glcContextState::isContext(i))
      break;
  }

  if (i == GLC_MAX_CONTEXTS) {
    /* All the contexts are used. We can not generate a new one => ERROR !!! */
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    __glcContextState::unlock();
    return 0;
  }

  /* Create a new context */
  state = new __glcContextState(i);
  if (!state) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    __glcContextState::unlock();
    return 0;
  }

  /* Check if the GLC_PATH environment variable is exported */
  if (getenv("GLC_PATH")) {
    char *path = NULL;
    char *begin = NULL;
    char *sep = NULL;

    /* Read the paths to fonts file. GLC_PATH uses the same format than PATH */
    path = strdup(getenv("GLC_PATH"));
    if (path) {

      /* Get each path and add the corresponding masters to the current context */
      begin = path;
      do {
	sep = (char *)__glcFindIndexList(begin, 1, ":");
	*(sep - 1) = 0;
	state->addMasters(begin, GL_TRUE);
	begin = sep;
      } while (*sep);
      free(path);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return 0;
    }
  }

  __glcContextState::unlock();

  return i + 1;
}

/** \ingroup global
  * Returns a zero terminated array of GLC context IDs that contains one entry
  * for each of the client's GLC contexts. GLC uses the ISO C library command
  * <b>malloc</b> to allocate the array. The client should use the ISO C library
  * command <b>free</b> to deallocate the array when it is no longer needed.
  */
GLint* glcGetAllContexts(void)
{
  int count = 0;
  int i = 0;
  GLint* contextArray = NULL;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  /* Count the number of existing contexts (whether they are current to a thread
   * or not).
   */
  for (i=0; i < GLC_MAX_CONTEXTS; i++) {
    if (__glcContextState::isContext(i))
      count++;
  }

  /* Allocate memory to store the array and raise an error if this fails */
  contextArray = (GLint *)malloc(sizeof(GLint) * count);
  if (!contextArray) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Copy the context IDs to the array */
  for (i = GLC_MAX_CONTEXTS - 1; i >= 0; i--) {
    if (__glcContextState::isContext(i))
      contextArray[--count] = i;
  }

  return contextArray;
}

/** \ingroup global
  * retrieves the value of the issuing thread's GLC error code variable, assigns
  * the value <b>GLC_NONE</b> to that variable, and returns the retrieved value.
  */
GLCenum glcGetError(void)
{
  GLCenum error = GLC_NONE;

  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);

  error = (GLCenum)pthread_getspecific(__glcContextState::errorKey);
  __glcContextState::raiseError(GLC_NONE);
  return error;
}
