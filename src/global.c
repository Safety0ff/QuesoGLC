/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
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
 *  Commands to create, manage and destroy GLC contexts.
 *
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
 *  threads. With the exception of the per-thread GLC error code and context ID
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

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#define GLCAPI __declspec(dllexport)
#endif

#include "internal.h"
#include <stdlib.h>
#include FT_LIST_H

#ifdef QUESOGLC_STATIC_LIBRARY
pthread_once_t __glcInitLibraryOnce = PTHREAD_ONCE_INIT;
#endif



/* Since the common area can be accessed by any thread, this function should
 * be called before any access (read or write) to the common area. Otherwise
 * race conditons can occur.
 * __glcLock/__glcUnlock can be nested : they keep track of the number of
 * time they have been called and the mutex will be released as soon as
 * __glcUnlock() will be called as many time as __glcLock().
 */
static void __glcLock(void)
{
  __GLCthreadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  if (!area->lockState)
#ifdef __WIN32__
    EnterCriticalSection(&__glcCommonArea.section);
#else
    pthread_mutex_lock(&__glcCommonArea.mutex);
#endif

  area->lockState++;
}



/* Unlock the mutex in order to allow other threads to amke accesses to the
 * common area.
 * See also the note on nested calls in __glcLock's description.
 */
static void __glcUnlock(void)
{
  __GLCthreadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  area->lockState--;
  if (!area->lockState)
#ifdef __WIN32__
    LeaveCriticalSection(&__glcCommonArea.section);
#else
    pthread_mutex_unlock(&__glcCommonArea.mutex);
#endif
}



/* This function is called each time a pthread is cancelled or exits in order
 * to free its specific area
 */
static void __glcFreeThreadArea(void *keyValue)
{
  __GLCthreadArea *area = (__GLCthreadArea*)keyValue;
  __GLCcontext *ctx = NULL;

  if (area) {
    /* Release the context which is current to the thread, if any */
    ctx = area->currentContext;
    if (ctx)
      ctx->isCurrent = GL_FALSE;
    free(area); /* DO NOT use __glcFree() !!! */
  }
}



/* This function is called when QuesoGLC is no longer needed.
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcExitLibrary(void)
#else
#ifdef __GNUC__
__attribute__((destructor)) void fini(void)
#else
void _fini(void)
#endif
#endif
{
  FT_ListNode node = NULL;
#if 0
  void *key = NULL;
#endif

  __glcLock();

  /* destroy remaining contexts */
  node = __glcCommonArea.contextList.head;
  while (node) {
    FT_ListNode next = node->next;
    __glcCtxDestroy((__GLCcontext*)node);
    node = next;
  }

  __glcUnlock();
#ifdef __WIN32__
  DeleteCriticalSection(&__glcCommonArea.section);
#else
  pthread_mutex_destroy(&__glcCommonArea.mutex);
#endif

#if 0
  /* Destroy the thread local storage */
  key = pthread_getspecific(__glcCommonArea.threadKey);
  if (key)
    __glcFreeThreadArea(key);

  pthread_key_delete(__glcCommonArea.threadKey);
#endif

#if FC_MINOR > 2
  FcFini();
#endif
}



/* Routines for memory management of FreeType
 * The memory manager of our FreeType library class uses the same memory
 * allocation functions than QuesoGLC
 */
static void* __glcAllocFunc(FT_Memory inMemory, long inSize)
{
  GLC_DISCARD_ARG(inMemory);
  return __glcMalloc(inSize);
}

static void __glcFreeFunc(FT_Memory inMemory, void *inBlock)
{
  GLC_DISCARD_ARG(inMemory);
  __glcFree(inBlock);
}

static void* __glcReallocFunc(FT_Memory inMemory, long inCurSize,
			      long inNewSize, void* inBlock)
{
  GLC_DISCARD_ARG(inMemory);
  GLC_DISCARD_ARG(inCurSize);
  return __glcRealloc(inBlock, inNewSize);
}



/* This function is called before any function of QuesoGLC
 * is used. It reserves memory and initialiazes the library, hence the name.
 */
#ifdef QUESOGLC_STATIC_LIBRARY
static void __glcInitLibrary(void)
#else
#ifdef __GNUC__
__attribute__((constructor)) void init(void)
#else
void _init(void)
#endif
#endif
{
  /* Initialize fontconfig */
  if (!FcInit())
    goto FatalError;

  __glcCommonArea.versionMajor = 0;
  __glcCommonArea.versionMinor = 2;

  /* Create the thread-local storage for GLC errors */
#ifdef __WIN32__
  __glcCommonArea.threadKey = TlsAlloc();
  if (__glcCommonArea.threadKey == 0xffffffff)
    goto FatalError;
#else
  if (pthread_key_create(&__glcCommonArea.threadKey, __glcFreeThreadArea))
    goto FatalError;
#endif

  __glcCommonArea.memoryManager.user = NULL;
  __glcCommonArea.memoryManager.alloc = __glcAllocFunc;
  __glcCommonArea.memoryManager.free = __glcFreeFunc;
  __glcCommonArea.memoryManager.realloc = __glcReallocFunc;

  /* Initialize the list of context states */
  __glcCommonArea.contextList.head = NULL;
  __glcCommonArea.contextList.tail = NULL;

  /* Initialize the mutex for access to the contextList array */
#ifdef __WIN32__
  InitializeCriticalSection(&__glcCommonArea.section);
#else
  if (pthread_mutex_init(&__glcCommonArea.mutex, NULL))
    goto FatalError;
#endif

#ifdef QUESOGLC_STATIC_LIBRARY
  atexit(__glcExitLibrary);
#endif
  return;

 FatalError:
  __glcRaiseError(GLC_RESOURCE_ERROR);

  /* Is there a better thing to do than that ? */
  perror("GLC Fatal Error");
  exit(-1);
}



#if defined __WIN32__ && !defined __GNUC__
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
  switch(dwReason) {
  case DLL_PROCESS_ATTACH:
    _init();
	return TRUE;
  case DLL_PROCESS_DETACH:
    _fini();
	return TRUE;
  }
  return TRUE;
}
#endif



/* Get the context state corresponding to a given context ID */
static __GLCcontext* __glcGetContext(GLint inContext)
{
  FT_ListNode node = NULL;

  __glcLock();
  for (node = __glcCommonArea.contextList.head; node; node = node->next)
    if (((__GLCcontext*)node)->id == inContext) break;

  __glcUnlock();

  return (__GLCcontext*)node;
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
GLboolean APIENTRY glcIsContext(GLint inContext)
{
#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  return (__glcGetContext(inContext) ? GL_TRUE : GL_FALSE);
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
GLint APIENTRY glcGetCurrentContext(void)
{
  __GLCcontext *ctx = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  ctx = __glcGetCurrent();
  if (!ctx)
    return 0;
  else
    return ctx->id;
}



/** \ingroup global
 *  Marks for deletion the GLC context identified by \e inContext. If the
 *  marked context is not current to any client thread, the command deletes
 *  the marked context immediatly. Otherwise, the marked context will be
 *  deleted during the execution of the next glcContext() command that causes
 *  it not to be current to any client thread.
 *
 *  \note glcDeleteContext() does not destroy the GL objects associated with
 *  the context \e inContext. Indeed for performance reasons, GLC does not keep
 *  track of the GL context that contains the GL objects associated with the
 *  the GLC context that is destroyed. Even if GLC would keep track of the GL
 *  context, it could lead GLC to temporarily change the GL context, delete the
 *  GL objects, then restore the correct GL context. In order not to adversely
 *  impact the performance of most of programs, it is the responsability of the
 *  user to call glcDeleteGLObjects() on a GLC context that is intended to be
 *  destroyed.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inContext is not the ID of
 *  one of the client's GLC contexts.
 *  \param inContext The ID of the context to be deleted
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
void APIENTRY glcDeleteContext(GLint inContext)
{
  __GLCcontext *ctx = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  /* verify if the context exists */
  ctx = __glcGetContext(inContext);

  if (!ctx) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    __glcUnlock();
    return;
  }

  if (ctx->isCurrent)
    /* The context is current to a thread : just mark for deletion */
    ctx->pendingDelete = GL_TRUE;
  else {
    /* Remove the context from the context list then destroy it */
    FT_List_Remove(&__glcCommonArea.contextList, (FT_ListNode)ctx);
    __glcCtxDestroy(ctx);
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
 *  When a GLCcontext is made current to a thread, GLC issues the commands
 *  \code
 *  glGetString(GL_VERSION);
 *  glGetString(GL_EXTENSIONS);
 *  \endcode
 *  and stores the returned strings. If there is no GL context current to the
 *  thread, the result of the above GL commands is undefined and so is the
 *  result of glcContext().
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
void APIENTRY glcContext(GLint inContext)
{
  char *version = NULL;
  char *extension = NULL;
  __GLCcontext *currentState = NULL;
  __GLCcontext *ctx = NULL;
  __GLCthreadArea *area = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  if (inContext < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  area = __glcGetThreadArea();
  assert(area);

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  if (inContext) {
    /* verify that the context exists */
    ctx = __glcGetContext(inContext);

    if (!ctx) {
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
    if (ctx->isCurrent) {
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
    area->currentContext = ctx;
    ctx->isCurrent = GL_TRUE;
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
      FT_List_Remove(&__glcCommonArea.contextList, (FT_ListNode)currentState);
      __glcCtxDestroy(currentState);
    }
  }

  __glcUnlock();

  /* If the current context was released then there is no need to check OpenGL
   * extensions.
   */
  if (!inContext)
    return;

  version = (char *)glGetString(GL_VERSION);
  extension = (char *)glGetString(GL_EXTENSIONS);

  if (version && extension && (glewInit() != GLEW_OK))
    __glcRaiseError(GLC_RESOURCE_ERROR);
}



/** \ingroup global
 *  Generates a new GLC context and returns its ID.
 *  \return The ID of the new context
 *  \sa glcGetAllContexts()
 *  \sa glcIsContext()
 *  \sa glcContext()
 *  \sa glcGetCurrentContext()
 */
GLint APIENTRY glcGenContext(void)
{
  int newContext = 0;
  __GLCcontext *ctx = NULL;
  FT_ListNode node = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Lock the "Common Area" in order to prevent race conditions */
  __glcLock();

  /* Search for the first context ID that is unused */
  ctx = (__GLCcontext*)__glcCommonArea.contextList.tail;
  if (!ctx)
    newContext = 1;
  else
    newContext = ctx->id + 1;

  /* Create a new context */
  ctx = __glcCtxCreate(newContext);
  if (!ctx) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcUnlock();
    return 0;
  }

  node = (FT_ListNode)ctx;
  node->data = ctx;
  FT_List_Add(&__glcCommonArea.contextList, node);

  __glcUnlock();

  /* The environment variable GLC_PATH is an alternate way to allow QuesoGLC
   * to access to fonts catalogs/directories.
   */
  /*Check if the GLC_PATH environment variable is exported */
  if (getenv("GLC_CATALOG_LIST") || getenv("GLC_PATH")) {
    char *path = NULL;
    char *begin = NULL;
    char *sepPos = NULL;
    char *separator = NULL;

    /* Read the paths of fonts file.
     * First, try GLC_CATALOG_LIST...
     */
    if (getenv("GLC_CATALOG_LIST"))
      path = strdup(getenv("GLC_CATALOG_LIST"));
    else if (getenv("GLC_PATH")) {
      /* Try GLC_PATH which uses the same format than PATH */
      path = strdup(getenv("GLC_PATH"));
    }

    /* Get the list separator */
    separator = getenv("GLC_LIST_SEPARATOR");
    if (!separator) {
#ifdef __WIN32__
	/* Windows can not use a colon-separated list since the colon sign is
	 * used after the drive letter. The semicolon is used for the PATH
	 * variable, so we use it for consistency.
	 */
	separator = (char *)";";
#else
	/* POSIX platforms uses colon-separated lists for the paths variables
	 * so we keep with it for consistency.
	 */
	separator = (char *)":";
#endif
    }

    if (path) {
      /* Get each path and add the corresponding masters to the current
       * context */
      begin = path;
      do {
	sepPos = (char *)__glcFindIndexList(begin, 1, separator);

        if (*sepPos)
	  *(sepPos++) = 0;
	if (!FcStrSetAdd(ctx->catalogList, (const FcChar8*)begin))
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	if (!FcConfigAppFontAddDir(ctx->config, (const unsigned char*)begin)) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  FcStrSetDel(ctx->catalogList, (const FcChar8*)begin);
	}
	begin = sepPos;
      } while (*sepPos);
      __glcFree(path);
      __glcUpdateHashTable(ctx);
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
GLint* APIENTRY glcGetAllContexts(void)
{
  int count = 0;
  GLint* contextArray = NULL;
  FT_ListNode node = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif

  /* Count the number of existing contexts (whether they are current to a
   * thread or not).
   */
  __glcLock();
  for (node = __glcCommonArea.contextList.head, count = 0; node;
       node = node->next, count++);

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
  for (node = __glcCommonArea.contextList.tail; node;node = node->prev)
    contextArray[--count] = ((__GLCcontext*)node)->id;

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
GLCenum APIENTRY glcGetError(void)
{
  GLCenum error = GLC_NONE;
  __GLCthreadArea * area = NULL;

#ifdef QUESOGLC_STATIC_LIBRARY
  pthread_once(&__glcInitLibraryOnce, __glcInitLibrary);
#endif
  area = __glcGetThreadArea();
  assert(area);

  error = area->errorState;
  __glcRaiseError(GLC_NONE);
  return error;
}
