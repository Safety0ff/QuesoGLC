/** \file global.c
  * \brief Global Commands
  */
/* $Id$ */
#include <stdlib.h>
#include <stdio.h>

#include "GL/glc.h"
#include "ocontext.h"
#include "internal.h"

/** \ingroup global
  * Returns <b>GL_TRUE</b> if <i>inContext</i> is the ID of one of the client's
  * GLC contexts.
  */
GLboolean glcIsContext(GLint inContext)
{
    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

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
    
    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

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

  pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

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

  if (__glcContextState::getCurrency(inContext - 1))
    state->pendingDelete = GL_TRUE;
  else
    delete state;
}

/** \ingroup global
  * Assigns the value <i>inContext</i> to the issuing thread's current GLC context
  * ID variable. The command raises <b>GLC_PARAMETER_ERROR</b> if <i>inContext</i>
  * is not zero and is not the ID of one of the client's GLC contexts.
  * The command raises <b>GLC_STATE_ERROR</b> if <i>inContext</i> is the ID of a
  * GLC context that is current to a thread other than the issuing thread.
  * The command raises <b>GLC_STATE_ERROR</b> if the issuing thread is executing
  * a callback function that has been called from GLC.
  */
void glcContext(GLint inContext)
{
    char *version = NULL;
    char *extension = NULL;
    
    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

    if (inContext) {
	GLint currentContext = 0;
	
	/* verify parameters are in legal bounds */
	if ((inContext < 0) || (inContext > GLC_MAX_CONTEXTS)) {
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
	}
	/* verify that the context has been created */
	if (!__glcContextState::isContext(inContext - 1)) {
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
	}
	
	currentContext = glcGetCurrentContext();
	
	/* Check if glcGenContext has been called from within a callback
	 * function that has been called from GLC
	 */
	if (currentContext) {
	  __glcContextState *state = __glcContextState::getCurrent();
	    if (state->isInCallbackFunc) {
		__glcContextState::raiseError(GLC_STATE_ERROR);
		return;
	    }
	}
	
	/* Is the context already current ? */
	if (__glcContextState::getCurrency(inContext - 1)) {
	    /* If the context is current to another thread -> ERROR ! */
	    if (currentContext != inContext)
		__glcContextState::raiseError(GLC_STATE_ERROR);
	    
	    /* Anyway the context is already current to one thread
	       then return */
	    return;
	}
	
	/* Release old current context if any */
	if (currentContext) {
	    __glcContextState *state = NULL;
	    
	    __glcContextState::setCurrency(currentContext - 1, GL_FALSE);
	    state = __glcContextState::getState(currentContext - 1);
	    
	    if (!state)
		__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	    else {
		/* execute pending deletion if any */
		if (state->pendingDelete)
		    delete state;
	    }
	}
	
	/* Make the context current to the thread */
	pthread_setspecific(__glcContextState::contextKey, (__glcContextState::stateList)[inContext - 1]);
	__glcContextState::setCurrency(inContext - 1, GL_TRUE);
    }
    else {
	GLint currentContext = 0;
	
	currentContext = glcGetCurrentContext();
	
	if (currentContext) {
	    pthread_setspecific(__glcContextState::contextKey, NULL);
	    __glcContextState::setCurrency(currentContext - 1, GL_FALSE);
	}
    }
    
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
    char *path = NULL;
    char *begin = NULL;
    char *sep = NULL;

    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

    for (i=0 ; i<GLC_MAX_CONTEXTS; i++) {
	if (!__glcContextState::isContext(i))
	    break;
    }
    
    if (i == GLC_MAX_CONTEXTS) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
    
    state = new __glcContextState(i);
    if (!state) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return 0;
    }

    // Read the GLC_PATH environment variable
    if (getenv("GLC_PATH")) {
      path = strdup(getenv("GLC_PATH"));
      if (path) {
	begin = path;
	do {
	  sep = (char *)__glcFindIndexList(begin, 1, ":");
	  *(sep - 1) = 0;
	  state->addMasters(begin, GL_TRUE);
	  begin = sep;
	} while (*sep);
	free(path);
      }
      else
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
    }

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

    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

    for (i=0; i < GLC_MAX_CONTEXTS; i++) {
	if (__glcContextState::isContext(i))
	    count++;
    }
    
    contextArray = (GLint *)malloc(sizeof(GLint) * count);
    if (!contextArray) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return NULL;
    }

    for (i=0; i < GLC_MAX_CONTEXTS; i++) {
	if (__glcContextState::isContext(i))
	    contextArray[--count] = i;
    }
    
    return contextArray;
}

/** \ingroup global
  * retrieves the value of the issuing thread's GLC error code variable, assigns the
  * value <b>GLC_NONE</b> to that variable, and returns the retrieved value.
  */
GLCenum glcGetError(void)
{
    GLCenum error = GLC_NONE;
    pthread_once(&__glcContextState::initOnce, __glcContextState::initQueso);

    error = (GLCenum)pthread_getspecific(__glcContextState::errorKey);
    __glcContextState::raiseError(GLC_NONE);
    return error;
}

void my_fini(void)
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
}
