/** \file global.c
  * \brief Global Commands
  */
/* $Id$ */
#include <pthread.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"

FT_Library library;

static GLboolean *__glcContextIsCurrent = NULL;
static __glcContextState **__glcContextStateList = NULL;
static pthread_once_t __glcInitThreadOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t __glcCommonAreaMutex;
static pthread_key_t __glcContextKey;
static pthread_key_t __glcErrorKey;
char __glcBuffer[GLC_STRING_CHUNK];
GDBM_FILE unicod1, unicod2;

static void __glcInitThread(void)
{
    pthread_mutex_init(&__glcCommonAreaMutex, NULL);
    if (pthread_key_create(&__glcContextKey, NULL)) {
	/* Initialisation has failed. What do we do ? */
    }
    if (pthread_key_create(&__glcErrorKey, NULL)) {
	/* Initialisation has failed. What do we do ? */
    }
    if (FT_Init_FreeType(&library)) {
	/* Initialisation has failed. What do we do ? */
    }
}

__glcContextState* __glcGetCurrentState(void)
{
    return (__glcContextState *)pthread_getspecific(__glcContextKey);
}

void __glcRaiseError(GLCenum inError)
{
    GLCenum error = GLC_NONE;

    /* An error can only be raised if the current error value is GLC_NONE. 
       However, when inError == GLC_NONE then we must force the current error
       value to GLC_NONE whatever was its previous value */
    error = (GLCenum)pthread_getspecific(__glcErrorKey);
    if ((inError && !error) || !inError)
	pthread_setspecific(__glcErrorKey, (void *)inError);
}

static GLboolean __glcGetContextCurrency(GLint inContext)
{
    GLboolean isCurrent = GL_FALSE;

    pthread_mutex_lock(&__glcCommonAreaMutex);
    isCurrent = __glcContextIsCurrent[inContext];
    pthread_mutex_unlock(&__glcCommonAreaMutex);

    return isCurrent;
}

static void __glcSetContextCurrency(GLint inContext, GLboolean isCurrent)
{
    pthread_mutex_lock(&__glcCommonAreaMutex);
    __glcContextIsCurrent[inContext] = isCurrent;
    pthread_mutex_unlock(&__glcCommonAreaMutex);
}

static __glcContextState* __glcGetContextState(GLint inContext)
{
    __glcContextState *state = NULL;

    pthread_mutex_lock(&__glcCommonAreaMutex);
    state = __glcContextStateList[inContext];
    pthread_mutex_unlock(&__glcCommonAreaMutex);
    
    return state;
}

static void __glcSetContextState(GLint inContext, __glcContextState *state)
{
    pthread_mutex_lock(&__glcCommonAreaMutex);
    __glcContextStateList[inContext] = state;
    pthread_mutex_unlock(&__glcCommonAreaMutex);
}

static GLboolean __glcIsContext(GLint inContext)
{
    __glcContextState *state = NULL;
    
    pthread_mutex_lock(&__glcCommonAreaMutex);
    state = __glcContextStateList[inContext];
    pthread_mutex_unlock(&__glcCommonAreaMutex);
    
    return (state ? GL_TRUE : GL_FALSE);
}

/** \ingroup global
  * Returns <b>GL_TRUE</b> if <i>inContext</i> is the ID of one of the client's
  * GLC contexts.
  */
GLboolean glcIsContext(GLint inContext)
{
    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS))
	return GL_FALSE;
    else
	return __glcIsContext(inContext - 1);
}

/** \ingroup global
  * Returns the value of the issueing thread's current GLC context ID variable
  */
GLint glcGetCurrentContext(void)
{
    __glcContextState *state = NULL;
    
    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    state = (__glcContextState *)pthread_getspecific(__glcContextKey);
    if (!state)
	return 0;
    else
	return state->id;
}

static void __glcDeleteContext(GLint inContext)
{
    __glcContextState *state = NULL;
    int i = 0;
    
    __glcRaiseError(GLC_NONE);
    pthread_mutex_lock(&__glcCommonAreaMutex);
    state = __glcContextStateList[inContext];
    __glcContextStateList[inContext] = NULL;
    pthread_mutex_unlock(&__glcCommonAreaMutex);

    delete state->catalogList;
    for (i = 0; i < GLC_MAX_FONT; i++) {
	__glcFont *font = NULL;
	
	font = state->fontList[i];
	if (font) {
	    glcDeleteFont(i + 1);
	}
    }
    
    for (i = 0; i < GLC_MAX_MASTER; i++) {
	if (state->masterList[i])
	    __glcDeleteMaster(i, state);
    }
    
    glcDeleteGLObjects();

    free(state);
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
    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    /* verify parameters are in legal bounds */
    if ((inContext < 1) || (inContext > GLC_MAX_CONTEXTS)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return;
    }
    /* verify that the context has been created */
    if (!__glcIsContext(inContext - 1)) {
	__glcRaiseError(GLC_PARAMETER_ERROR);
	return;
    }
    
    if (__glcGetContextCurrency(inContext - 1))
	__glcGetContextState(inContext - 1)->pendingDelete = GL_TRUE;
    else
	__glcDeleteContext(inContext - 1);
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
    
    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    if (inContext) {
	GLint currentContext = 0;
	
	/* verify parameters are in legal bounds */
	if ((inContext < 0) || (inContext > GLC_MAX_CONTEXTS)) {
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return;
	}
	/* verify that the context has been created */
	if (!__glcIsContext(inContext - 1)) {
	    __glcRaiseError(GLC_PARAMETER_ERROR);
	    return;
	}
	
	currentContext = glcGetCurrentContext();
	
	/* Check if glcGenContext has been called from within a callback
	 * function that has been called from GLC
	 */
	if (currentContext) {
	    __glcContextState *state = __glcGetCurrentState();
	    
	    if (state->isInCallbackFunc) {
		__glcRaiseError(GLC_STATE_ERROR);
		return;
	    }
	}
	
	/* Is the context already current ? */
	if (__glcGetContextCurrency(inContext - 1)) {
	    /* If the context is current to another thread -> ERROR ! */
	    if (currentContext != inContext)
		__glcRaiseError(GLC_STATE_ERROR);
	    
	    /* Anyway the context is already current to one thread
	       then return */
	    return;
	}
	
	/* Release old current context if any */
	if (currentContext) {
	    __glcContextState *state = NULL;
	    
	    __glcSetContextCurrency(currentContext - 1, GL_FALSE);
	    state = __glcGetContextState(currentContext - 1);
	    
	    if (!state)
		__glcRaiseError(GLC_INTERNAL_ERROR);
	    else {
		/* execute pending deletion if any */
		if (state->pendingDelete)
		    __glcDeleteContext(currentContext - 1);
	    }
	}
	
	/* Make the context current to the thread */
	pthread_setspecific(__glcContextKey, __glcContextStateList[inContext - 1]);
	__glcSetContextCurrency(inContext - 1, GL_TRUE);
    }
    else {
	GLint currentContext = 0;
	
	currentContext = glcGetCurrentContext();
	
	if (currentContext) {
	    pthread_setspecific(__glcContextKey, NULL);
	    __glcSetContextCurrency(currentContext - 1, GL_FALSE);
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
    int j = 0;
    __glcContextState *state = NULL;

    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    for (i=0 ; i<GLC_MAX_CONTEXTS; i++) {
	if (!__glcIsContext(i))
	    break;
    }
    
    if (i == GLC_MAX_CONTEXTS) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
    
    state = (__glcContextState *)malloc(sizeof(__glcContextState));
    if (!state) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return 0;
    }
    __glcSetContextState(i, state);
    __glcSetContextCurrency(i, GL_FALSE);
    
    state->catalogList = new __glcStringList(GLC_UCS1);
    if (!state->catalogList) {
	__glcSetContextState(i, NULL);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	free(state);
	return 0;
    }
    
    state->id = i + 1;
    state->pendingDelete = GL_FALSE;
    state->callback = GLC_NONE;
    state->dataPointer = NULL;
    state->autoFont = GL_TRUE;
    state->glObjects = GL_TRUE;
    state->mipmap = GL_TRUE;
    state->resolution = 0.;
    state->bitmapMatrix[0] = 1.;
    state->bitmapMatrix[1] = 0.;
    state->bitmapMatrix[2] = 0.;
    state->bitmapMatrix[3] = 1.;
    state->currentFontCount = 0;
    state->fontCount = 0;
    state->listObjectCount = 0;
    state->masterCount = 0;
    state->measuredCharCount = 0;
    state->renderStyle = GLC_BITMAP;
    state->replacementCode = 0;
    state->stringType = GLC_UCS1;
    state->textureObjectCount = 0;
    state->versionMajor = 0;
    state->versionMinor = 2;
    state->isInCallbackFunc = GL_FALSE;
    
    for (j=0; j < GLC_MAX_CURRENT_FONT; j++)
	state->currentFontList[j] = 0;

    for (j=0; j < GLC_MAX_FONT; j++)
	state->fontList[j] = NULL;

    for (j=0; j < GLC_MAX_LIST_OBJECT; j++)
	state->listObjectList[j] = 0;

    for (j=0; j < GLC_MAX_MASTER; j++)
	state->masterList[j] = NULL;

    for (j=0; j < GLC_MAX_TEXTURE_OBJECT; j++)
	state->textureObjectList[j] = 0;

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

    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    for (i=0; i < GLC_MAX_CONTEXTS; i++) {
	if (__glcIsContext(i))
	    count++;
    }
    
    contextArray = (GLint *)malloc(sizeof(GLint) * count);
    if (!contextArray) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
    }

    for (i=0; i < GLC_MAX_CONTEXTS; i++) {
	if (__glcIsContext(i))
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
    pthread_once(&__glcInitThreadOnce, __glcInitThread);

    error = (GLCenum)pthread_getspecific(__glcErrorKey);
    __glcRaiseError(GLC_NONE);
    return error;
}

void my_init(void)
{
    int i = 0;
   
    /* create Common Area */
    __glcContextIsCurrent = (GLboolean *)malloc(sizeof(GLboolean) * GLC_MAX_CONTEXTS);
    if (!__glcContextIsCurrent)
	return;

    __glcContextStateList = (__glcContextState **)malloc(sizeof(__glcContextState*) * GLC_MAX_CONTEXTS);
    if (!__glcContextStateList) {
	free(__glcContextIsCurrent);
	return;
    }
   
    for (i=0; i< GLC_MAX_CONTEXTS; i++) {
	__glcContextIsCurrent[i] = GL_FALSE;
	__glcContextStateList[i] = NULL;
    }
    
    unicod1 = gdbm_open("database/unicode1.db", 0, GDBM_READER, 0, NULL);
    unicod2 = gdbm_open("database/unicode2.db", 0, GDBM_READER, 0, NULL);
}

void my_fini(void)
{
    int i;

    /* destroy remaining contexts */
    for (i=0; i < GLC_MAX_CONTEXTS; i++) {
    if (__glcIsContext(i))
	    __glcDeleteContext(i);
    }

    /* destroy Common Area */
    free(__glcContextIsCurrent);
    free(__glcContextStateList);
    
    gdbm_close(unicod1);
    gdbm_close(unicod2);
}
