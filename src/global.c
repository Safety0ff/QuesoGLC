/** \file global.c
  * \brief Global Commands
  */
/* $Id$ */
#include <stdlib.h>
#include <stdio.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

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

#ifdef _REENTRANT
  pthread_mutex_destroy(&__glcContextState::mutex);
#endif
}

/* This function is called each time a pthread is cancelled or exits in order
 * to free its specific area
 */
#ifdef _REENTRANT
static void __glcFreeThreadArea(void *keyValue)
{
  threadArea *area = (threadArea*)keyValue;

  if (area)
    __glcFree(area);
}
#endif

/* This function is supposed to be called before any of QuesoGLC
 * function is used. It reserves memory and opens the Unicode DB files.
 */
void __glcInitLibrary(void)
{
  int i = 0;

  // Creates the thread-local storage for the GLC error
#ifdef _REENTRANT
  if (pthread_key_create(&__glcContextState::threadKey, __glcFreeThreadArea))
    goto FatalError;
#else
  __glcContextState::initOnce = GL_TRUE;
#endif

  // Initializes the "Common Area"

  // Creates the array of state currency
  __glcContextState::isCurrent = new GLboolean[GLC_MAX_CONTEXTS];
  if (!__glcContextState::isCurrent) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    goto FatalError;
  }

  // Creates the array of context states
  __glcContextState::stateList = new __glcContextState*[GLC_MAX_CONTEXTS];
  if (!__glcContextState::stateList) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    delete[] __glcContextState::isCurrent;
    goto FatalError;
  }

  // Open the first Unicode database
  __glcContextState::unidb1 = gdbm_open("database/unicode1.db", 0, GDBM_READER, 0, NULL);
  if (!__glcContextState::unidb1) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    goto FatalError;
  }

  // Open the second Unicode database
  __glcContextState::unidb2 = gdbm_open("database/unicode2.db", 0, GDBM_READER, 0, NULL);
  if (!__glcContextState::unidb2) {
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    gdbm_close(__glcContextState::unidb1);
    delete[] __glcContextState::isCurrent;
    delete[] __glcContextState::stateList;
    goto FatalError;
  }

  // Initialize the mutex
  // At least this op can not fail !!!
#ifdef _REENTRANT
  pthread_mutex_init(&__glcContextState::mutex, NULL);
#endif

  // So far, there are no contexts
  for (i=0; i< GLC_MAX_CONTEXTS; i++) {
    __glcContextState::isCurrent[i] = GL_FALSE;
    __glcContextState::stateList[i] = NULL;
  }

  atexit(__glcExitLibrary);
  return;

 FatalError:
  perror("GLC Fatal Error");
  exit(-1);
}

/* glcIsContext:
 *   This command returns GL_TRUE if inContext is the ID of one of the client's
 *   GLC contexts.
 */
GLboolean glcIsContext(GLint inContext)
{
#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif

  if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS))
    return GL_FALSE;
  else
    return __glcContextState::isContext(inContext - 1);
}

/* glcGetCurrentContext:
 *   Returns the value of the issueing thread's current GLC context ID variable
 */
GLint glcGetCurrentContext(void)
{
  __glcContextState *state = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif

  state = __glcContextState::getCurrent();
  if (!state)
    return 0;
  else
    return state->id;
}

/* glcDeleteContext:
 *   Marks for deletion the GLC context identified by <i>inContext</i>. If the
 *   marked context is not current to any client thread, the command deletes
 *   the marked context immediatly. Otherwise, the marked context will be
 *   deleted during the execution of the next glcContext() command that causes
 *   it not to be current to any client thread. The command raises
 *   GLC_PARAMETER_ERROR if inContext is not the ID of one of the client's GLC
 *    contexts.
 */
void glcDeleteContext(GLint inContext)
{
  __glcContextState *state = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif

  /* verify if parameters are in legal bounds */
  if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS)) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  state = __glcContextState::getState(inContext - 1);

  /* verify if the context has been created */
  if (!state) {
    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
    return;
  }

  __glcContextState::lock();

  if (__glcContextState::getCurrency(inContext - 1))
    /* The context is current to a thread : just mark for deletion */
    state->pendingDelete = GL_TRUE;
  else {
    delete state;
    __glcContextState::stateList[inContext - 1] = NULL;
  }

  __glcContextState::unlock();
}

/* glcContext:
 *   Assigns the value inContext to the issuing thread's current GLC context ID
 *   variable. The command raises GLC_PARAMETER_ERROR if inContext is not zero
 *   and is not the ID of one of the client's GLC contexts. The command raises
 *   GLC_STATE_ERROR if inContext is the ID of a GLC context that is current to
 *   a thread other than the issuing thread. The command raises GLC_STATE_ERROR
 *   if the issuing thread is executing a callback function that has been
 *   called from GLC.
 */
void glcContext(GLint inContext)
{
  char *version = NULL;
  char *extension = NULL;
  __glcContextState *state = NULL;
  GLint currentContext = 0;
  threadArea * area = NULL;
  Display *dpy = NULL;
  Screen *screen = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif
  area = __glcContextState::getThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist.
     */
    perror("GLC Fatal Error");
    exit(-1);
  }

  /* Get the screen on which drawing ops will be performed */
  dpy = glXGetCurrentDisplay();
  if (!dpy) {
    /* No GLX context is associated with the current thread. */
    __glcContextState::raiseError(GLC_RESOURCE_ERROR);
    return;
  }
  /* WARNING ! This may not be relevant if the display has several screens */
  screen = DefaultScreenOfDisplay(dpy);

  /* Lock the "Common Area" in order to prevent race conditions */
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
      /* If the context is current to another thread => ERROR ! */
      if (currentContext != inContext)
	__glcContextState::raiseError(GLC_STATE_ERROR);

      /* If we get there, this means that the context 'inContext'
       * is already current to one thread (whether it is the issuing thread
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
    area->currentContext = __glcContextState::stateList[inContext - 1];
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
      area->currentContext = NULL;
      /* Release the context */
      __glcContextState::setCurrency(currentContext - 1, GL_FALSE);
    }
  }

  /* execute pending deletion if any */
  if (currentContext && state) {
    if (state->pendingDelete) {
      delete state;
      __glcContextState::stateList[currentContext - 1] = NULL;
    }
  }

  __glcContextState::unlock();

  /* We read the version and extensions of the OpenGL client. We do it
   * for compliance with the specifications because we do not use it.
   * However it may be useful if QuesoGLC tries to use some GL commands 
   * that are not part of OpenGL 1.0
   */
  version = (char *)glGetString(GL_VERSION);
  extension = (char *)glGetString(GL_EXTENSIONS);

  /* Compute the resolution of the screen in DPI (dots per inch) */
  if (WidthMMOfScreen(screen) && HeightMMOfScreen(screen)) {
    area->currentContext->displayDPIx = (GLuint)( 25.4 * WidthOfScreen(screen) / WidthMMOfScreen(screen));
    area->currentContext->displayDPIy = (GLuint) (25.4 * HeightOfScreen(screen) / HeightMMOfScreen(screen));
  }
  else {
    area->currentContext->displayDPIx = 72;
    area->currentContext->displayDPIy = 72;
  }
}

/* glcGenContext:
 *   Generates a new GLC context and returns its ID.
 */
GLint glcGenContext(void)
{
  int i = 0;
  __glcContextState *state = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
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

  __glcContextState::stateList[i] = state;

  __glcContextState::unlock();

  /* Check if the GLC_PATH environment variable is exported */
  if (getenv("GLC_PATH")) {
    char *path = NULL;
    char *begin = NULL;
    char *sep = NULL;

    /* Read the paths of fonts file. GLC_PATH uses the same format than PATH */
    path = strdup(getenv("GLC_PATH"));
    if (path) {

      /* Get each path and add the corresponding masters to the current
       * context */
      begin = path;
      do {
	sep = (char *)__glcFindIndexList(begin, 1, ":");
	*(sep - 1) = 0;
	state->addMasters(begin, GL_TRUE);
	begin = sep;
      } while (*sep);
      __glcFree(path);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcContextState::raiseError(GLC_RESOURCE_ERROR);
      return 0;
    }
  }

  return i + 1;
}

/* glcGetAllContexts:
 *   Returns a zero terminated array of GLC context IDs that contains one entry
 *   for each of the client's GLC contexts. GLC uses the ISO C library command
 *   malloc to allocate the array. The client should use the ISO C library
 *   command free to deallocate the array when it is no longer needed.
 */
GLint* glcGetAllContexts(void)
{
  int count = 0;
  int i = 0;
  GLint* contextArray = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif

  /* Count the number of existing contexts (whether they are current to a
   * thread or not).
   */
  for (i=0; i < GLC_MAX_CONTEXTS; i++) {
    if (__glcContextState::isContext(i))
      count++;
  }

  /* Allocate memory to store the array */
  contextArray = (GLint *)__glcMalloc(sizeof(GLint) * count);
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

/* glcGetError:
 *   retrieves the value of the issuing thread's GLC error code variable,
 *   assigns the value GLC_NONE to that variable, and returns the retrieved
 *   value.
 */
GLCenum glcGetError(void)
{
  GLCenum error = GLC_NONE;
  threadArea * area = NULL;

#ifdef _REENTRANT
  pthread_once(&__glcContextState::initLibraryOnce, __glcInitLibrary);
#else
  if (!__glcContextState::initOnce)
    __glcInitLibrary();
#endif
  area = __glcContextState::getThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist.
     */
    perror("GLC Fatal Error");
    exit(-1);
  }

  error = area->errorState;
  __glcContextState::raiseError(GLC_NONE);
  return error;
}
