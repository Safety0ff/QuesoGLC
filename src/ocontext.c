/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, Bertrand Coconnier
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
#include <stdio.h>

#include "ocontext.h"
#include "internal.h"
#include FT_MODULE_H
#include FT_LIST_H

commonArea *__glcCommonArea = NULL;

__glcContextState* __glcCtxCreate(GLint inContext)
{
  GLint j = 0;
  __glcContextState *This = NULL;

  This = (__glcContextState*)__glcMalloc(sizeof(__glcContextState));

  __glcSetState(inContext, This);
  __glcSetCurrency(inContext, GL_FALSE);

  if (FT_New_Library(__glcCommonArea->memoryManager, &This->library)) {
    __glcSetState(inContext, NULL);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    This->library = NULL;
    return NULL;
  }
  FT_Add_Default_Modules(This->library);

  This->catalogList = __glcStrLstCreate(NULL);
  if (!This->catalogList) {
    __glcSetState(inContext, NULL);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    This->library = NULL;
    return NULL;
  }

  This->id = inContext + 1;
  This->pendingDelete = GL_FALSE;
  This->callback = GLC_NONE;
  This->dataPointer = NULL;
  This->autoFont = GL_TRUE;
  This->glObjects = GL_TRUE;
  This->mipmap = GL_TRUE;
  This->resolution = 0.05;
  This->bitmapMatrix[0] = 1.;
  This->bitmapMatrix[1] = 0.;
  This->bitmapMatrix[2] = 0.;
  This->bitmapMatrix[3] = 1.;
  This->currentFontCount = 0;
  This->fontCount = 0;
  This->listObjectCount = 0;
  This->masterCount = 0;
  This->measuredCharCount = 0;
  This->renderStyle = GLC_BITMAP;
  This->replacementCode = 0;
  This->stringType = GLC_UCS1;
  This->textureObjectCount = 0;
  This->versionMajor = 0;
  This->versionMinor = 2;
  This->isInCallbackFunc = GL_FALSE;

  for (j=0; j < GLC_MAX_CURRENT_FONT; j++)
    This->currentFontList[j] = 0;

  for (j=0; j < GLC_MAX_FONT; j++)
    This->fontList[j] = NULL;

  for (j=0; j < GLC_MAX_MASTER; j++)
    This->masterList[j] = NULL;

  for (j=0; j < GLC_MAX_TEXTURE_OBJECT; j++)
    This->textureObjectList[j] = 0;

  This->buffer = NULL;
  This->bufferSize = 0;

  return This;
}

void __glcCtxDestroy(__glcContextState *This)
{
  int i = 0;

  __glcStrLstDestroy(This->catalogList);

  for (i = 0; i < GLC_MAX_FONT; i++) {
    __glcFont *font = NULL;

    font = This->fontList[i];
    if (font) {
      glcDeleteFont(i + 1);
    }
  }

  for (i = 0; i < GLC_MAX_MASTER; i++) {
    if (This->masterList[i]) {
      __glcMasterDestroy(This->masterList[i]);
      This->masterList[i] = NULL;
    }
  }

  if (This->bufferSize)
    __glcFree(This->buffer);

  glcDeleteGLObjects();
  FT_Done_Library(This->library);
}

/* Get the current context of the issuing thread */
__glcContextState* __glcGetCurrent(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist. May be we should issue
     * a fatal error to stderr and kill the program ?
     */
    return NULL;
  }

  return area->currentContext;
}

/* Get the context state of a given context */
__glcContextState* __glcGetState(GLint inContext)
{
  __glcContextState *state = NULL;

  __glcLock();
  state = __glcCommonArea->stateList[inContext];
  __glcUnlock();

  return state;
}

/* Set the context state of a given context */
void __glcSetState(GLint inContext, __glcContextState *inState)
{
  __glcLock();
  __glcCommonArea->stateList[inContext] = inState;
  __glcUnlock();
}

/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcRaiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist. May be we should issue
     * a fatal error to stderr and kill the program ?
     */
    return;
  }

  /* An error can only be raised if the current error value is GLC_NONE. 
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = area->errorState;
  if ((inError && !error) || !inError)
    area->errorState = inError;
}

/* Determine whether a context is current to a thread or not */
GLboolean __glcGetCurrency(GLint inContext)
{
  GLboolean current = GL_FALSE;

  __glcLock();
  current = __glcCommonArea->isCurrent[inContext];
  __glcUnlock();

  return current;
}

/* Make a context current or release it. This value is intended to determine
 * if a context is current to a thread or not. Array isCurrent[] exists
 * because there is no simple way to get the contextKey value of
 * other threads (we do not even know how many threads have been created)
 */
void __glcSetCurrency(GLint inContext, GLboolean current)
{
  __glcLock();
  __glcCommonArea->isCurrent[inContext] = current;
  __glcUnlock();
}

/* Determine if a context ID is associated to a context state or not. This is
 * a different notion than the currency of a context : a context state may
 * exist without being associated to a thread.
 */
GLboolean __glcIsContext(GLint inContext)
{
  __glcContextState *state = __glcGetState(inContext);

  return (state ? GL_TRUE : GL_FALSE);
}

/* This function updates the GLC_CHAR_LIST list when a new face identified by
 * 'face' is added to the master pointed by inMaster
 */
static int __glcUpdateCharList(__glcMaster* inMaster, FT_Face face)
{
  FT_ULong charCode;
  FT_UInt gIndex;
  FT_List list = NULL;
  FT_ListNode node = NULL, current = NULL;
  FT_ULong *data = NULL;

  list = (FT_List)__glcMalloc(sizeof(FT_ListRec));
  if (!list)
    return -1;
  list->head = NULL;
  list->tail = NULL;


  charCode = FT_Get_First_Char(face, &gIndex);
  while (gIndex) {
    if (inMaster->minMappedCode > charCode)
      inMaster->minMappedCode = charCode;
    if (inMaster->maxMappedCode < charCode)
      inMaster->maxMappedCode = charCode;

    data = (FT_ULong*)__glcMalloc(sizeof(charCode));
    if (!data) {
      FT_List_Finalize(list, __glcListDestructor,
		       __glcCommonArea->memoryManager, NULL);
      __glcFree(list);
      return -1;
    }
    *data = charCode;

    node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
    if (!node) {
      __glcFree(data);
      FT_List_Finalize(list, __glcListDestructor,
		       __glcCommonArea->memoryManager, NULL);
      __glcFree(list);
      __glcFree(data);
      return -1;
    }
    node->data = data;

    FT_List_Add(list, node);
    charCode = FT_Get_Next_Char(face, charCode, &gIndex);
  }

  current = inMaster->charList->head;

  while(list->head && current){
    if (*((FT_ULong*)current->data) > *((FT_ULong*)list->head->data)) {
      if (current->prev) {
	if (*((FT_ULong*)current->prev->data) == *((FT_ULong*)list->head->data)) {
	  node = list->head;
	  FT_List_Remove(list, node);
	  __glcFree(node);
	  continue;
	}
      }
      node = list->head;
      FT_List_Remove(list, node);
      if (current->prev)
	current->prev->next = node;
      else
	inMaster->charList->head = node;
      node->prev = current->prev;
      node->next = current;
      current->prev = node;
      inMaster->charListCount++;
    }
    current = current->next;
  }

  while (list->head){
    node = list->head;
    FT_List_Remove(list, node);
    FT_List_Add(inMaster->charList, node);
    inMaster->charListCount++;
  }

  return 0;
}

/* This function is called by glcAppendCatalog and glcPrependCatalog. It has
 * been associated to the contextState class rather than the master class
 * because glcAppendCatalog and glcPrependCatalog might create several masters.
 * Moreover it associates the new masters (if any) to the current context.
 */
void __glcCtxAddMasters(__glcContextState *This, const GLCchar* inCatalog,
			GLboolean inAppend)
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
    __glcRaiseError(GLC_RESOURCE_ERROR);
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
    if (!FT_New_Face(This->library, path, 0, &face)) {
      numFaces = face->num_faces;

      /* If the face has no Unicode charmap, skip it */
      if (FT_Select_Charmap(face, ft_encoding_unicode))
	continue;

      /* Determine if the family (i.e. "Times", "Courier", ...) is already
       * associated to a master.
       */
      for (j = 0; j < This->masterCount; j++) {
	s.ptr = face->family_name;
	s.type = GLC_UCS1;
	if (!__glcUniCompare(&s, This->masterList[j]->family))
	  break;
      }

      if (j < This->masterCount)
	/* We have found the master corresponding to the family of the current
	 * font file.
	 */
	master = This->masterList[j];
      else {
	/* No master has been found. We must create one */

	if (This->masterCount < GLC_MAX_MASTER - 1) {
	  /* Create a new master and add it to the current context */
	  master = __glcMasterCreate(face, desc, ext, This->masterCount,
				     This->stringType);
	  if (!master) {
	    __glcRaiseError(GLC_RESOURCE_ERROR);
	    continue;
	  }
	  /* FIXME : use the first free location instead of this one */
	  This->masterList[This->masterCount++] = master;
	}
	else {
	  /* We have already created the maximum number of masters allowed */
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  continue;
	}
      }
      FT_Done_Face(face);
    }
    else {
      /* FreeType is not able to open the font file, try the next one */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      continue;
    }

    /* For each face in the font file */
    for (j = 0; j < numFaces; j++) {
      __glcUniChar sp;

      sp.ptr = path;
      sp.type = GLC_UCS1;
      if (!FT_New_Face(This->library, path, j, &face)) {
	s.ptr = face->style_name;
	s.type = GLC_UCS1;
	if (!__glcStrLstFind(master->faceList, &s))
	  /* The current face in the font file is not already loaded in a
	   * master : Append (or prepend) the new face and its file name to
	   * the master.
	   */
	  /* FIXME : 
	   *  If there are several faces into the same file then we
	   *  should indicate it inside faceFileName
	   */
	  if (!__glcUpdateCharList(master, face)) {
	    if (inAppend) {
	      if (__glcStrLstAppend(master->faceList, &s))
		break;
	      if (__glcStrLstAppend(master->faceFileName, &sp)) {
		__glcStrLstRemove(master->faceList, &s);
		break;		    
	      }
	    }
	    else {
	      if (__glcStrLstPrepend(master->faceList, &s))
		break;
	      if (__glcStrLstPrepend(master->faceFileName, &sp)) {
		__glcStrLstRemove(master->faceList, &s);
		break;		    
	      }
	    }
	  }
	}

      else {
      /* FreeType is not able to read this face in the font file, 
       * try the next one.
       */
	__glcRaiseError(GLC_RESOURCE_ERROR);
	continue;
      }
      FT_Done_Face(face);
    }
  }

  /* Append (or prepend) the directory name to the catalog list */
  s.ptr = (GLCchar*)inCatalog;
  s.type = GLC_UCS1;
  if (inAppend) {
    if (__glcStrLstAppend(This->catalogList, &s)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }
  else {
    if (__glcStrLstPrepend(This->catalogList, &s)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }

  /* Closes 'fonts.dir' */
  fclose(file);
}

/* This function is called by glcRemoveCatalog. It has been included in the
 * context state class for the same reasons than addMasters was.
 */
void __glcCtxRemoveMasters(__glcContextState *This, GLint inIndex)
{
  const char* fileName = "/fonts.dir";
  char buffer[256];
  char path[256];
  int numFontFiles = 0;
  int i = 0;
  FILE *file;
  __glcUniChar s;

  /* TODO : use Unicode instead of ASCII */
  strncpy(buffer, (const char*)__glcStrLstFindIndex(This->catalogList,
						    inIndex), 256);
  strncpy(path, buffer, 256);
  strncat(path, fileName, strlen(fileName));

  /* Open 'fonts.dir' */
  file = fopen(path, "r");
  if (!file) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
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
      __glcMaster *master = This->masterList[j];
      if (!master)
	continue;
      s.ptr = buffer;
      s.type = GLC_UCS1;
      index = __glcStrLstGetIndex(master->faceFileName, &s);
      if (!index)
	continue; /* The file is not in the current master, try the next one */

      /* Removes the corresponding faces in the font list */
      for (i = 0; i < GLC_MAX_FONT; i++) {
	__glcFont *font = This->fontList[i];
	if (font) {
	  if ((font->parent == master) && (font->faceID == index)) {
	    /* Eventually remove the font from the current font list */
	    for (j = 0; j < GLC_MAX_CURRENT_FONT; j++) {
	      if (This->currentFontList[j] == i) {
		memmove(&This->currentFontList[j], &This->currentFontList[j+1],
			This->currentFontCount-j-1);
		This->currentFontCount--;
		break;
	      }
	    }
	    glcDeleteFont(i + 1);
	  }
	}
      }

      __glcStrLstRemoveIndex(master->faceFileName, index); /* Remove the file name */
      __glcStrLstRemoveIndex(master->faceList, index); /* Remove the face */

      /* FIXME :Characters from the font should be removed from the char list */

      /* If the master is empty (i.e. does not contain any face) then
       * remove it.
       */
      if (!master->faceFileName->count) {
	__glcMasterDestroy(master);
	master = NULL;
	This->masterCount--;
      }
    }
  }

  fclose(file);
  return;
}

/* Return the ID of the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'
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
GLint __glcCtxGetFont(__glcContextState *This, GLint inCode)
{
  GLint font = 0;

  font = __glcLookupFont(inCode, This);
  if (font)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(inCode, This)) {
    font = __glcLookupFont(inCode, This);
    if (font)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (This->autoFont) {
    GLint i = 0;

    for (i = 0; i < This->fontCount; i++) {
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
    for (i = 0; i < This->masterCount; i++) {
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

void __glcLock(void)
{
#ifdef _REENTRANT
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist. May be we should issue
     * a fatal error to stderr and kill the program ?
     */
    return;
  }

  if (!area->lockState)
    pthread_mutex_lock(&__glcCommonArea->mutex);

  area->lockState++;
#endif
}

void __glcUnlock(void)
{
#ifdef _REENTRANT
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  if (!area) {
    /* This is a severe problem : we can not even issue an error
     * since the threadArea does not exist. May be we should issue
     * a fatal error to stderr and kill the program ?
     */
    return;
  }

  area->lockState--;
  if (!area->lockState)
    pthread_mutex_unlock(&__glcCommonArea->mutex);
#endif
}

GLCchar* __glcCtxQueryBuffer(__glcContextState *This,int inSize)
{
  if (inSize > This->bufferSize) {
    This->buffer = (GLCchar*)__glcRealloc(This->buffer, inSize);
    if (!This->buffer)
      __glcRaiseError(GLC_RESOURCE_ERROR);
    else
      This->bufferSize = inSize;
  }

  return This->buffer;
}

threadArea* __glcGetThreadArea(void)
{
  threadArea *area = NULL;

#ifdef _REENTRANT
  area = (threadArea*)pthread_getspecific(__glcCommonArea->threadKey);
#else
  area = __glcCommonArea->area;
#endif

  if (!area) {
    area = (threadArea*)__glcMalloc(sizeof(threadArea));
    if (!area)
      return NULL;

    area->currentContext = NULL;
    area->errorState = GLC_NONE;
    area->lockState = 0;
#ifdef _REENTRANT
    pthread_setspecific(__glcCommonArea->threadKey, (void*)area);
#endif
  }

  return area;
}
