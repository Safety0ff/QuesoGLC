/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */

/** \file
 *  defines the so-called "Global commands" described in chapter 3.4 of the
 *  GLC specs.
 */

/** \defgroup global Global Commands
 *  Those commands do not use GLC context state variables and can therefore be
 *  executed successfully if the issuing thread has no current GLC context. 
 *
 *  Each GLC context has a nonzero ID of type \b GLint. When a client is linked
 *  with a GLC library, the library maintains a list of IDs that contains one
 *  entry for each of the client's GLC contexts. The list is initially empty.
 *
 *  Each client thread has a private GLC context ID variable that always
 *  contains either the value zero, indicating that the thread has no current
 *  GLC context, or the ID of the thread's current GLC context. The initial
 *  value is zero.
 *
 *  When the ID of a GLC context is stored in the GLC context ID variable of a
 *  client thread, the context is said to be current to the thread. It is not
 *  possible for a GLC context to be current simultaneously to multiple
 *  threads.With the exception of the per-thread GLC error code and context ID
 *  variables, all of the GLC state variables that are used during the
 *  execution of a GLC command are stored in the issuing thread's current GLC
 *  context. To make a context current, call glcContext().
 *
 *  When a client thread issues a GLC command, the thread's current GLC context
 *  executes the command.
 *
 *  Note that the results of issuing a GL command when there is no current GL
 *  context are undefined. Because GLC issues GL commands, you must create a GL
 *  context and make it current before calling GLC.
 *
 *  All other GLC commands raise \b GLC_STATE_ERROR if the issuing thread has
 *  no current GLC context.
 */

#include <stdlib.h>
#include <stdio.h>
#ifndef __APPLE__
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif
#include <fcntl.h>
#include <assert.h>
#include <fontconfig/fontconfig.h>

#include "GL/glc.h"
#include "internal.h"
#include "ocontext.h"
#include FT_LIST_H

#ifdef QUESOGLC_STATIC_LIBRARY
pthread_once_t __glcInitLibraryOnce = PTHREAD_ONCE_INIT;
#endif



/* This function is called from FT_List_Finalize() to destroy all
 * remaining contexts
 */
static void __glcStateDestructor(FT_Memory memory, void *data, void *user)
{
  __glcContextState *state = (__glcContextState*) data;

  if (state)
    __glcCtxDestroy(state);
}



/* This function is called when QuesoGLC is no longer needed.
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcExitLibrary(void)
#else
void _fini(void)
#endif
{
  __glcLock();

  /* destroy remaining contexts */
  FT_List_Finalize(&__glcCommonArea.stateList, __glcStateDestructor,
		   &__glcCommonArea.memoryManager, NULL);

  /* destroy Common Area */
  FcStrSetDestroy(__glcCommonArea.catalogList);

  __glcUnlock();
  pthread_mutex_destroy(&__glcCommonArea.mutex);
}



/* This function is called each time a pthread is cancelled or exits in order
 * to free its specific area
 */
static void __glcFreeThreadArea(void *keyValue)
{
  threadArea *area = (threadArea*)keyValue;
  __glcContextState *state = NULL;

  if (area) {
    /* Release the context which is current to the thread, if any */
    state = area->currentContext;
    if (state)
      state->isCurrent = GL_FALSE;
    free(area); /* DO NOT use __glcFree() !!! */
  }
}



/* Routines for memory management of FreeType
 * The memory manager of our FreeType library class uses the same memory
 * allocation functions than QuesoGLC
 */
static void* __glcAllocFunc(FT_Memory inMemory, long inSize)
{
  return __glcMalloc(inSize);
}

static void __glcFreeFunc(FT_Memory inMemory, void *inBlock)
{
  __glcFree(inBlock);
}

static void* __glcReallocFunc(FT_Memory inMemory, long inCurSize,
			      long inNewSize, void* inBlock)
{
  return __glcRealloc(inBlock, inNewSize);
}



/* This function is called before any function of QuesoGLC
 * is used. It reserves memory and initialiazes the library, hence the name.
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcInitLibrary(void)
#else
void _init(void)
#endif
{
  /* Initialize fontconfig */
  if (!FcInit())
    goto FatalError;

  __glcCommonArea.versionMajor = 0;
  __glcCommonArea.versionMinor = 2;

  /* Create the thread-local storage for GLC errors */
  if (pthread_key_create(&__glcCommonArea.threadKey, __glcFreeThreadArea))
    goto FatalError;

  __glcCommonArea.memoryManager.user = NULL;
  __glcCommonArea.memoryManager.alloc = __glcAllocFunc;
  __glcCommonArea.memoryManager.free = __glcFreeFunc;
  __glcCommonArea.memoryManager.realloc = __glcReallocFunc;

  /* Initialize the list of context states */
  __glcCommonArea.stateList.head = NULL;
  __glcCommonArea.stateList.tail = NULL;

  __glcCommonArea.catalogList = FcStrSetCreate();
  if (!__glcCommonArea.catalogList)
    goto FatalError;

  /* Initialize the mutex for access to the stateList array */
  if (pthread_mutex_init(&__glcCommonArea.mutex, NULL))
    goto FatalError;

#ifdef QUESOGLC_STATIC_LIBRARY
  atexit(__glcExitLibrary);
#endif
  return;

 FatalError:
  __glcRaiseError(GLC_RESOURCE_ERROR);

  if (__glcCommonArea.catalogList)
    FcStrSetDestroy(__glcCommonArea.catalogList);

  /* Is there a better thing to do than that ? */
  perror("GLC Fatal Error");
  exit(-1);
}



/** \ingroup global
 *  This command checks whether \e inContext is the ID of one of the client's
 *  GLC context and returns \b GLC_TRUE if and only if it is.
 *  \param inContext The context ID to be tested
 *  \return \b GL_TRUE if \e inContext is the ID of a GLC context,
 *          \b GL_FALSE otherwise
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcContext()
 */
GLboolean glcIsContext(GLint inContext)
{
#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  return (__glcGetState(inContext) ? GL_TRUE : GL_FALSE);
}



/** \ingroup global
 *  Returns the value of the issuing thread's current GLC context ID variable
 *  \return The context ID of the current thread
 *  \sa glcContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 */
GLint glcGetCurrentContext(void)
{
  __glcContextState *state = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  state = __glcGetCurrent();
  if (!state)
    return 0;
  else
    return state->id;
}



/** \ingroup global
 *  Marks for deletion the GLC context identified by \e inContext. If the
 *  marked context is not current to any client thread, the command deletes
 *  the marked context immediatly. Otherwise, the marked context will be
 *  deleted during the execution of the next glcContext() command that causes
 *  it not to be current to any client thread.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inContext is not the ID of
 *  one of the client's GLC contexts.
 *  \param inContext The ID of the context to be deleted
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
void glcDeleteContext(GLint inContext)
{
  __glcContextState *state = NULL;
  FT_ListNode node;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  /* verify if the context exists */
  state = __glcGetState(inContext);

  if (!state) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcUnlock();
    return;
  }

  if (state->isCurrent)
    /* The context is current to a thread : just mark for deletion */
    state->pendingDelete = GL_TRUE;
  else {
    node = FT_List_Find(&__glcCommonArea.stateList, state);
    assert(node); /* If node is not found then something went wrong ... */
    FT_List_Remove(&__glcCommonArea.stateList, node);
    __glcCtxDestroy(state);
    __glcFree(node);
  }

  __glcUnlock();
}



/** \ingroup global
 *  Assigns the value \e inContext to the issuing thread's current GLC context
 *  ID variable. If another context is already current to the thread, no error
 *  is generated but the context is released and the context identified by
 *  \e inContext is made current to the thread.
 *
 *  Call \e glcContext with \e inContext set to zero to release a thread's
 *  current context.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inContext is not zero
 *  and is not the ID of one of the client's GLC contexts. \n
 *  The command raises \b GLC_STATE_ERROR if \e inContext is the ID of a GLC
 *  context that is current to a thread other than the issuing thread. \n
 *  The command raises \b GLC_STATE_ERROR if the issuing thread is executing
 *  a callback function that has been called from GLC.
 *  \param inContext The ID of the context to be made current
 *  \sa glcGetCurrentContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 */
void glcContext(GLint inContext)
{
#if 0
  char *version = NULL;
  char *extension = NULL;
#endif
#ifndef __APPLE__
  Display *dpy = NULL;
  Screen *screen = NULL;
#endif
  __glcContextState *currentState = NULL;
  __glcContextState *state = NULL;
  threadArea *area = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  if (inContext < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  area = __glcGetThreadArea();
  assert(area);

#ifndef __APPLE__
  /* Get the screen on which drawing ops will be performed */
  dpy = glXGetCurrentDisplay();
  if (dpy) {
    /* WARNING ! This may not be relevant if the display has several screens */
    screen = DefaultScreenOfDisplay(dpy);
  }
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  if (inContext) {
    /* verify that the context exists */
    state = __glcGetState(inContext);

    if (!state) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      __glcUnlock();
      return;
    }

    /* Gets the current context state */
    currentState = area->currentContext;

    /* Check if the issuing thread is executing a callback
     * function that has been called from GLC
     */
    if (currentState) {
      if (currentState->isInCallbackFunc) {
	__glcRaiseError(GLC_STATE_ERROR);
	__glcUnlock();
	return;
      }
    }

    /* Is the context already current to a thread ? */
    if (state->isCurrent) {
      /* If the context is current to another thread => ERROR ! */
      if (!currentState)
	__glcRaiseError(GLC_STATE_ERROR);
      else
	if (currentState->id != inContext)
	  __glcRaiseError(GLC_STATE_ERROR);

      /* If we get there, this means that the context 'inContext'
       * is already current to one thread (whether it is the issuing thread
       * or not) : there is nothing else to be done.
       */
      __glcUnlock();
      return;
    }

    /* Release old current context if any */
    if (currentState)
      currentState->isCurrent = GL_FALSE;

    /* Make the context current to the thread */
    area->currentContext = state;
    state->isCurrent = GL_TRUE;
  }
  else {
    /* inContext is null, the current thread must release its context if any */

    /* Gets the current context state */
    currentState = area->currentContext;

    if (currentState) {
      /* Deassociate the context from the issuing thread */
      area->currentContext = NULL;
      /* Release the context */
      currentState->isCurrent = GL_FALSE;
    }
  }

  /* execute pending deletion if any */
  if (currentState) {
    if (currentState->pendingDelete) {
      FT_ListNode node = FT_List_Find(&__glcCommonArea.stateList,
				      currentState);
      assert(node); /* If node is not found then something went wrong ... */
      FT_List_Remove(&__glcCommonArea.stateList, node);
      __glcCtxDestroy(currentState);
      __glcFree(node);
    }
  }

  __glcUnlock();

#if 0
  /* We read the version and extensions of the OpenGL client. We do it
   * for compliance with the specifications because we do not use it.
   * However it may be useful if QuesoGLC tries to use some GL commands 
   * that are not part of OpenGL 1.0
   */
  version = (char *)glGetString(GL_VERSION);
  extension = (char *)glGetString(GL_EXTENSIONS);
#endif

  if (area->currentContext) {
#ifndef __APPLE__
    if (dpy) {
      /* Compute the resolution of the screen in DPI (dots per inch) */
      if (WidthMMOfScreen(screen) && HeightMMOfScreen(screen)) {
	area->currentContext->displayDPIx =
	  (GLuint)( 25.4 * WidthOfScreen(screen) / WidthMMOfScreen(screen));
	area->currentContext->displayDPIy =
	  (GLuint) (25.4 * HeightOfScreen(screen) / HeightMMOfScreen(screen));
      }
      return;
    }
#endif
    /* Standard values */
    area->currentContext->displayDPIx = 72;
    area->currentContext->displayDPIy = 72;
  }
}



/** \ingroup global
 *  Generates a new GLC context and returns its ID.
 *  \return The ID of the new context
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
GLint glcGenContext(void)
{
  int newContext = 0;
  __glcContextState *state = NULL;
  FT_ListNode node = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  /* Search for the first context ID that is unused */
  node = __glcCommonArea.stateList.tail;
  if (!node) {
    newContext = 1;
  }
  else {
    state = (__glcContextState *)node->data;
    newContext = state->id + 1;
  }

  /* Create a new context */
  state = __glcCtxCreate(newContext);
  if (!state) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return 0;
  }

  node = (FT_ListNode) __glcMalloc(sizeof(FT_ListNodeRec));
  if (!node) {
    __glcCtxDestroy(state);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return 0;
  }

  node->data = state;
  FT_List_Add(&__glcCommonArea.stateList, node);

  __glcUnlock();

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
        if (--sep != begin + strlen(begin))
	  *(sep++) = 0;
	__glcCtxAddMasters(state, begin, GL_TRUE);
	begin = sep;
      } while (*sep);
      __glcFree(path);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
  }

  return newContext;
}



/** \ingroup global
 *  Returns a zero terminated array of GLC context IDs that contains one entry
 *  for each of the client's GLC contexts. GLC uses the ISO C library command
 *  \c malloc to allocate the array. The client should use the ISO C library
 *  command \c free to deallocate the array when it is no longer needed.
 *  \return The pointer to the array of context IDs.
 *  \sa glcContext()
 *  \sa glcDeleteContext()
 *  \sa glcGenContext()
 *  \sa glcGetCurrentContext()
 *  \sa glcIsContext()
 */
GLint* glcGetAllContexts(void)
{
  int count = 0;
  GLint* contextArray = NULL;
  FT_ListNode node = NULL;
  __glcContextState *state = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Count the number of existing contexts (whether they are current to a
   * thread or not).
   */
  __glcLock();
  for (node = __glcCommonArea.stateList.head, count = 0; node;
       node = node->next, count++) {}

  /* Allocate memory to store the array */
  contextArray = (GLint *)__glcMalloc(sizeof(GLint) * (count+1));
  if (!contextArray) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return NULL;
  }

  /* Array must be null-terminated */
  contextArray[count] = 0;

  /* Copy the context IDs to the array */
  for (node = __glcCommonArea.stateList.tail; node;node = node->prev) {
    state = (__glcContextState *)node->data;
    assert(state);
    contextArray[--count] = state->id;
  }

  __glcUnlock();

  return contextArray;
}



/** \ingroup global
 *  Retrieves the value of the issuing thread's GLC error code variable,
 *  assigns the value \b GLC_NONE to that variable, and returns the retrieved
 *  value.
 *  \note In contrast to the GL function \c glGetError, \e glcGetError only
 *  returns one error, not a list of errors.
 *  \return An error code from the table below : \n\n
 *   <center>
 *   <table>
 *     <caption>Error codes</caption>
 *     <tr>
 *       <td>Name</td> <td>Enumerant</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_NONE</b></td> <td>0x0000</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_PARAMETER_ERROR</b></td> <td>0x0040</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_RESOURCE_ERROR</b></td> <td>0x0041</td>
 *     </tr>
 *     <tr>
 *       <td><b>GLC_STATE_ERROR</b></td> <td>0x0042</td>
 *     </tr>
 *   </table>
 *   </center>
 */
GLCenum glcGetError(void)
{
  GLCenum error = GLC_NONE;
  threadArea * area = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  area = __glcGetThreadArea();
  assert(area);

  error = area->errorState;
  __glcRaiseError(GLC_NONE);
  return error;
}
