/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

/* This file defines the so-called "Global commands" described in chapter 3.4
 * of the GLC specs. These are commands which do not use GLC context state
 * variables and which can therefore be executed succesfully if the issuing
 * thread has no current GLC context. All other GLC commands raise
 * GLC_STATE_ERROR if the issuing thread has no current GLC context.
 */

#include <stdlib.h>
#include <stdio.h>
#ifndef __MACOSX__
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif
#include <fcntl.h>

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
 * It frees the memory and closes the Unicode DB files
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcExitLibrary(void)
#else
void _fini(void)
#endif
{
  if (!__glcCommonArea)
    return;

  __glcLock();

  /* destroy remaining contexts */
  FT_List_Finalize(__glcCommonArea->stateList, __glcStateDestructor,
		   __glcCommonArea->memoryManager, NULL);

  /* destroy Common Area */
  __glcFree(__glcCommonArea->stateList);
  __glcFree(__glcCommonArea->memoryManager);
  dbm_close(__glcCommonArea->unidb1);
  dbm_close(__glcCommonArea->unidb2);

  __glcUnlock();
  pthread_mutex_destroy(&__glcCommonArea->mutex);

  __glcFree(__glcCommonArea);
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

    __glcFree(area);
  }
}



/* Routines for memory management of FreeType */
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
 * is used. It reserves memory and opens the Unicode DB files.
 * In a few words, it initialiazes the library, hence the name
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcInitLibrary(void)
#else
void _init(void)
#endif
{
  /* Creates and initializes the "Common Area" */

  __glcCommonArea = (commonArea*)__glcMalloc(sizeof(commonArea));
  if (!__glcCommonArea)
    goto FatalError;

  __glcCommonArea->stateList = NULL;
  __glcCommonArea->unidb1 = NULL;
  __glcCommonArea->unidb2 = NULL;
  __glcCommonArea->memoryManager = NULL;

  /* Creates the thread-local storage for GLC errors */
  if (pthread_key_create(&__glcCommonArea->threadKey, __glcFreeThreadArea))
    goto FatalError;

  __glcCommonArea->memoryManager =
    (FT_Memory)__glcMalloc(sizeof(struct FT_MemoryRec_));
  if (!__glcCommonArea->memoryManager)
    goto FatalError;

  __glcCommonArea->memoryManager->user = NULL;
  __glcCommonArea->memoryManager->alloc = __glcAllocFunc;
  __glcCommonArea->memoryManager->free = __glcFreeFunc;
  __glcCommonArea->memoryManager->realloc = __glcReallocFunc;

  /* Creates the array of context states */
  __glcCommonArea->stateList = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  if (!__glcCommonArea->stateList)
    goto FatalError;
  __glcCommonArea->stateList->head = NULL;
  __glcCommonArea->stateList->tail = NULL;

  /* Open the first Unicode database */
  __glcCommonArea->unidb1 = dbm_open("database/unicode1", O_RDONLY, 0444);
  if (!__glcCommonArea->unidb1)
    goto FatalError;

  /* Open the second Unicode database */
  __glcCommonArea->unidb2 = dbm_open("database/unicode2", O_RDONLY, 0444);
  if (!__glcCommonArea->unidb2)
    goto FatalError;

  /* Initialize the mutex for access to the stateList array */
  if (pthread_mutex_init(&__glcCommonArea->mutex, NULL))
    goto FatalError;

  /* Initialize the mutex for access to the DB */
  if (pthread_mutex_init(&__glcCommonArea->dbMutex, NULL)) {
    pthread_mutex_destroy(&__glcCommonArea->mutex);
    goto FatalError;
  }

#ifdef QUESOGLC_STATIC_LIBRARY
  atexit(__glcExitLibrary);
#endif
  return;

 FatalError:
  __glcRaiseError(GLC_RESOURCE_ERROR);
  if (__glcCommonArea->memoryManager) {
    __glcFree(__glcCommonArea->memoryManager);
    __glcCommonArea->memoryManager = NULL;
  }
  if (__glcCommonArea->stateList) {
    __glcFree(__glcCommonArea->stateList);
    __glcCommonArea->stateList = NULL;
  }
  if (__glcCommonArea->unidb1) {
    dbm_close(__glcCommonArea->unidb1);
    __glcCommonArea->unidb1 = NULL;
  }
  if (__glcCommonArea->unidb2) {
    dbm_close(__glcCommonArea->unidb2);
    __glcCommonArea->unidb2 = NULL;
  }
  if (__glcCommonArea) {
    __glcFree(__glcCommonArea);
  }
  /* Is there a better thing to do than that ? */
  perror("GLC Fatal Error");
  exit(-1);
}



/* glcIsContext:
 *   This command returns GL_TRUE if inContext is the ID of one of the client's
 *   GLC contexts.
 */
GLboolean glcIsContext(GLint inContext)
{
#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  return (__glcGetState(inContext) ? GL_TRUE : GL_FALSE);
}



/* glcGetCurrentContext:
 *   Returns the value of the issuing thread's current GLC context ID variable
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



/* glcDeleteContext:
 *   Marks for deletion the GLC context identified by <i>inContext</i>. If the
 *   marked context is not current to any client thread, the command deletes
 *   the marked context immediatly. Otherwise, the marked context will be
 *   deleted during the execution of the next glcContext() command that causes
 *   it not to be current to any client thread. The command raises
 *   GLC_PARAMETER_ERROR if inContext is not the ID of one of the client's GLC
 *   contexts.
 */
void glcDeleteContext(GLint inContext)
{
  __glcContextState *state = NULL;
  FT_ListNode node;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  __glcLock();

  state = __glcGetState(inContext);

  /* verify if the context has been created */
  if (!state) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcUnlock();
    return;
  }

  if (state->isCurrent)
    /* The context is current to a thread : just mark for deletion */
    state->pendingDelete = GL_TRUE;
  else {
    node = FT_List_Find(__glcCommonArea->stateList, state);
    if (node) {
      FT_List_Remove(__glcCommonArea->stateList, node);
      __glcCtxDestroy(state);
      __glcFree(node);
    }
  }

  __glcUnlock();
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
#if 0
  char *version = NULL;
  char *extension = NULL;
#endif
#ifndef __MACOSX__
  Display *dpy = NULL;
  Screen *screen = NULL;
#endif
  __glcContextState *currentState = NULL;
  __glcContextState *state = NULL;
  threadArea * area = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  if (inContext < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist.
     */
    perror("GLC Fatal Error");
    exit(-1);
  }

#ifndef __MACOSX__
  /* Get the screen on which drawing ops will be performed */
  dpy = glXGetCurrentDisplay();
  if (!dpy) {
    /* No GLX context is associated with the current thread. */
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  /* WARNING ! This may not be relevant if the display has several screens */
  screen = DefaultScreenOfDisplay(dpy);
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  if (inContext) {
    /* verify that the context exists */
    if (!glcIsContext(inContext)) {
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

    state = __glcGetState(inContext);

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
      FT_ListNode node = FT_List_Find(__glcCommonArea->stateList,
				      currentState);
      if (node) {
	FT_List_Remove(__glcCommonArea->stateList, node);
	__glcCtxDestroy(currentState);
	__glcFree(node);
      }
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
#ifdef __MACOSX__
    /* Standard values */
    area->currentContext->displayDPIx = 72;
    area->currentContext->displayDPIy = 72;
#else
    /* Compute the resolution of the screen in DPI (dots per inch) */
    if (WidthMMOfScreen(screen) && HeightMMOfScreen(screen)) {
      area->currentContext->displayDPIx =
        (GLuint)( 25.4 * WidthOfScreen(screen) / WidthMMOfScreen(screen));
      area->currentContext->displayDPIy =
        (GLuint) (25.4 * HeightOfScreen(screen) / HeightMMOfScreen(screen));
    }
    else {
      /* Standard values */
      area->currentContext->displayDPIx = 72;
      area->currentContext->displayDPIy = 72;
    }
#endif
  }
}



/* glcGenContext:
 *   Generates a new GLC context and returns its ID.
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
  node = __glcCommonArea->stateList->tail;
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
  FT_List_Add(__glcCommonArea->stateList, node);

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
	*(sep - 1) = 0;
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



/* glcGetAllContexts:
 *   Returns a zero terminated array of GLC context IDs that contains one entry
 *   for each of the client's GLC contexts. GLC uses the ISO C library command
 *   malloc to allocate the array. The client should use the ISO C library
 *   command free to deallocate the array when it is no longer needed.
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
  node = __glcCommonArea->stateList->head;
  while (node) {
    count++;
    node = node->next;
  }

  /* Allocate memory to store the array */
  contextArray = (GLint *)malloc(sizeof(GLint) * (count+1));
  if (!contextArray) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return NULL;
  }

  /* Array must be null-terminated */
  contextArray[count] = 0;

  /* Copy the context IDs to the array */
  node = __glcCommonArea->stateList->tail;
  while (node) {
    state = (__glcContextState *)node->data;
    if (state)
      contextArray[--count] = state->id;
    node = node->prev;
  }

  __glcUnlock();

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

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist.
     */
    perror("GLC Fatal Error");
    exit(-1);
  }

  error = area->errorState;
  __glcRaiseError(GLC_NONE);
  return error;
}
