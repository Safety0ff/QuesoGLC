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

/* Defines the methods of an object that is intended to managed contexts */

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fontconfig/fontconfig.h>

#include "internal.h"
#include "ocontext.h"
#include FT_MODULE_H
#include FT_LIST_H

commonArea *__glcCommonArea = NULL;



/* Creates a new context : returns a struct that contains the GLC state
 * of the new context
 */
__glcContextState* __glcCtxCreate(GLint inContext)
{
  __glcContextState *This = NULL;

  This = (__glcContextState*)__glcMalloc(sizeof(__glcContextState));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__glcContextState));

  if (FT_New_Library(__glcCommonArea->memoryManager, &This->library))
    goto out_of_memory;
  FT_Add_Default_Modules(This->library);

  if (!__glcCreateList(&(This->masterList))) goto out_of_memory;
  if (!__glcCreateList(&(This->localCatalogList))) goto out_of_memory;
  if (!__glcCreateList(&(This->currentFontList))) goto out_of_memory;
  if (!__glcCreateList(&(This->fontList))) goto out_of_memory;

  This->isCurrent = GL_FALSE;
  This->id = inContext;
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
  This->measuredCharCount = 0;
  This->renderStyle = GLC_BITMAP;
  This->replacementCode = 0;
  This->stringType = GLC_UCS1;
  This->isInCallbackFunc = GL_FALSE;
  This->buffer = NULL;
  This->bufferSize = 0;

  return This;

out_of_memory:
  __glcRaiseError(GLC_RESOURCE_ERROR);
  if (This->fontList)
    __glcFree(This->fontList);
  if (This->currentFontList)
    __glcFree(This->currentFontList);
  if (This->masterList)
    __glcFree(This->masterList);
  if (This->library)
    FT_Done_Library(This->library);
  __glcFree(This);
  return NULL;    
}



/* This function is called from FT_List_Finalize() to destroy all
 * remaining masters
 */
static void __glcMasterDestructor(FT_Memory memory, void *data, void *user)
{
  __glcMaster *master = (__glcMaster*)data;

  if (master)
    __glcMasterDestroy(master);
}



/* This function is called from FT_List_Finalize() to destroy all
 * remaining fonts
 */
static void __glcFontDestructor(FT_Memory inMemory, void *inData, void* inUser)
{
  __glcFont *font = (__glcFont*)inData;
  __glcContextState* state = (__glcContextState*)inUser;
  
  assert(state);

  if (font)
    __glcFontDestroy(font);
}



/* Destroys a context : it first destroys all the objects (whether they are
 * internal GLC objects or GL textures or GL display lists) that have been
 * created during the life of the context. Then it releases the memory occupied
 * by the GLC state struct.
 */
void __glcCtxDestroy(__glcContextState *This)
{
  assert(This);

  /* Destroy GLC_CURRENT_FONT_LIST */
  FT_List_Finalize(This->currentFontList, NULL,
		   __glcCommonArea->memoryManager, NULL);
  __glcFree(This->currentFontList);

  /* Destroy GLC_FONT_LIST */
  FT_List_Finalize(This->fontList, __glcFontDestructor,
                   __glcCommonArea->memoryManager, This);
  __glcFree(This->fontList);

  /* Destroy the local list of catalogs */
  FT_List_Finalize(This->localCatalogList, NULL,
                   __glcCommonArea->memoryManager, NULL);
  __glcFree(This->localCatalogList);

  /* Must be called before the masters are destroyed since
   * the display lists are stored in the masters.
   */
  __glcDeleteGLObjects(This);

  /* destroy remaining masters */
  FT_List_Finalize(This->masterList, __glcMasterDestructor,
		   __glcCommonArea->memoryManager, NULL);
  __glcFree(This->masterList);

  if (This->bufferSize)
    __glcFree(This->buffer);

  FT_Done_Library(This->library);
  __glcFree(This);
}



/* Get the current context of the issuing thread */
__glcContextState* __glcGetCurrent(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  return area->currentContext;
}



/* Get the context state of a given context */
__glcContextState* __glcGetState(GLint inContext)
{
  FT_ListNode node = NULL;
  __glcContextState *state = NULL;

  __glcLock();
  for (node = __glcCommonArea->stateList->head; node; node = node->next) {
    state = (__glcContextState *)node->data;
    if (state) {
      if (state->id == inContext) break;
    }
    state = NULL;
  }
  __glcUnlock();

  return state;
}



/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcRaiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  /* An error can only be raised if the current error value is GLC_NONE. 
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = area->errorState;
  if ((inError && !error) || !inError)
    area->errorState = inError;
}



/* This function updates the GLC_CHAR_LIST list when a new face identified by
 * 'face' is added to the master pointed by inMaster
 */
static void __glcUpdateCharList(__glcMaster* inMaster, FcCharSet *charSet)
{
  FcChar32 charCode;
  FcChar32 base = 0;
  FcChar32 next = 0;
  FcChar32 prev_base = 0;
  FcChar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;

  /* Update the minimum mapped code */
  base = FcCharSetFirstPage(charSet, map, &next);
  for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
    if (map[i]) break;
  assert(i < FC_CHARSET_MAP_SIZE); /* If the map contains no char then
				    * something went wrong... */
  for (j = 0; j < 32; j++)
    if ((map[i] >> j) & 1) break;
  charCode = base + (i << 5) + j;
  if (inMaster->minMappedCode > charCode)
    inMaster->minMappedCode = charCode;

  /* Update the maximum mapped code */
  base = FcCharSetFirstPage(charSet, map, &next);
  do {
    prev_base = base;
    base = FcCharSetNextPage(charSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  for (i = FC_CHARSET_MAP_SIZE - 1; i >= 0; i--)
    if (map[i]) break;
  for (j = 31; j >= 0; j--)
    if ((map[i] >> j) & 1) break;
  charCode = prev_base + (i << 5) + j;
  if (inMaster->maxMappedCode < charCode)
    inMaster->maxMappedCode = charCode;

  /* Add the character set to the GLC_CHAR_LIST of inMaster */
  inMaster->charList = FcCharSetUnion(inMaster->charList, charSet);
}



/* This function is called by glcAppendCatalog and glcPrependCatalog. It has
 * been associated to the contextState class rather than the master class
 * because glcAppendCatalog and glcPrependCatalog might create several masters.
 * Moreover it associates the new masters (if any) to the current context.
 */
void __glcCtxAddMasters(__glcContextState *This, const GLCchar* inCatalog,
			GLboolean inAppend)
{
  /* Those buffers should be declared dynamically */
  int i = 0;
  FcConfig *config = FcConfigGetCurrent();
  struct stat dirStat;
  FcFontSet *fontSet = NULL;
  FcStrSet *subDirs = NULL;
  FcBool test;
  FcChar8* catalog = NULL;

  /* Verify that the catalog has not already been appended (or prepended) */
  __glcLock();
  test = FcStrSetMember(__glcCommonArea->catalogList, (FcChar8*)inCatalog);
  __glcUnlock();

  if (test)
    return;

  catalog = FcStrCopyFilename((const FcChar8*)inCatalog);
  if (!catalog)
    return;

  /* Check that 'path' points to a directory that can be read */
  if (access((char *)catalog, R_OK) < 0) {
    /* May be something more explicit should be done */
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }
  if (stat((char *)catalog, &dirStat) < 0) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }
  if (!S_ISDIR(dirStat.st_mode)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }

  fontSet = FcFontSetCreate();
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }
  
  subDirs = FcStrSetCreate();
  if (!subDirs) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }

  if (!FcDirScan(fontSet, subDirs, NULL, FcConfigGetBlanks(config),
		 (const unsigned char*)catalog, FcFalse)) {
    FcFontSetDestroy(fontSet);
    FcStrSetDestroy(subDirs);
    __glcFree(catalog);
    return;
  }
  FcStrSetDestroy(subDirs); /* Sub directories are not scanned */
  __glcFree(catalog);

  for (i = 0; i < fontSet->nfont; i++) {
    char *end = NULL;
    char *ext = NULL;
    __glcMaster *master = NULL;
    FcChar8 *fileName = NULL;
    FcChar8 *vendorName = NULL;
    FcChar8 *familyName = NULL;
    FcChar8 *styleName = NULL;
    FT_ListNode node = NULL;

    /* get the file name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName)
	== FcResultTypeMismatch)
      continue;

    /* get the vendor name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &vendorName)
	== FcResultTypeMismatch)
      vendorName = NULL;

    /* get the extension of the file */
    ext = (char*)fileName + (strlen((const char*)fileName) - 1);
    while ((*ext != '.') && (end - (char*)fileName))
      ext--;
    if (ext != (char*)fileName)
      ext++;

    /* Determine if the family (i.e. "Times", "Courier", ...) is already
     * associated to a master.
     */
    if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &familyName)
	== FcResultTypeMismatch)
      continue;
    for (node = This->masterList->head; node; node = node->next) {
      master = (__glcMaster*)node->data;

      if (!strcmp((const char*)familyName, (const char*)master->family))
	break;
    }

    if (!node) {
      /* No master has been found. We must create one */
      GLint id;
      int fixed;

      /* Create a new master and add it to the current context */
      if (master)
	/* master is already equal to the last master of masterList
	 * since the master list has been parsed in the "for" loop
	 * and no master has been found.
	 */
	id = master->id + 1;
      else
	id = 0;

      if (FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed)
	  == FcResultTypeMismatch)
	continue;

      master = __glcMasterCreate(familyName, vendorName, ext, id,
				 (fixed != FC_PROPORTIONAL),
				 This->stringType);
      if (!master) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	continue;
      }

      node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
      if (!node) {
	__glcMasterDestroy(master);
	master = NULL;
	__glcRaiseError(GLC_RESOURCE_ERROR);
	continue;
      }

      node->data = master;
      FT_List_Add(This->masterList, node);
    }

    /* FIXME :
     * If fontconfig is not able to get the style or the charset of a font
     * then the master creation is stopped. This may lead to void masters
     * if the master has just been created above. We should check if the
     * master needs to be destroyed since it won't contain a thing.
     */
    if (FcPatternGetString(fontSet->fonts[i], FC_STYLE, 0, &styleName)
	== FcResultTypeMismatch)
      continue;

    for (node = master->faceList->head; node; node = node->next) {
      __glcFaceDescriptor* faceDesc = NULL;
      
      assert(node->data);
      faceDesc = (__glcFaceDescriptor*)node->data;
      if (!strcmp((const char*)faceDesc->styleName, (const char*)styleName))
	break;
    }

    if (!node) {
      FcCharSet *charSet = NULL;
      __glcFaceDescriptor* faceDesc = NULL;
      int index = 0;
      FT_ListNode newNode = NULL;

      /* The current face in the font file is not already loaded in a
       * master : Append (or prepend) the new face and its file name to
       * the master.
       */
      if (FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet)
	  == FcResultTypeMismatch)
        continue;
      if (FcPatternGetInteger(fontSet->fonts[i], FC_INDEX, 0, &index)
	  == FcResultTypeMismatch)
	continue;

      __glcUpdateCharList(master, charSet);

      faceDesc = (__glcFaceDescriptor*)__glcMalloc(sizeof(__glcFaceDescriptor));
      if (!faceDesc) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
	FcFontSetDestroy(fontSet);
	return;
      }

      newNode = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
      if (!newNode) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
	__glcFree(faceDesc);
	FcFontSetDestroy(fontSet);
	return;
      }

      /* Filenames are kept in their original format which is supposed to
       * be compatible with strlen()
       */
      faceDesc->fileName = (FcChar8*)strdup((const char*)fileName);
      if (!faceDesc->fileName) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
	__glcFree(newNode);
	__glcFree(faceDesc);
	FcFontSetDestroy(fontSet);
	return;
      }
      faceDesc->styleName = (FcChar8*)strdup((const char*)styleName);
      if (!faceDesc->styleName) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
	__glcFree(faceDesc->fileName);
	__glcFree(newNode);
	__glcFree(faceDesc);
	FcFontSetDestroy(fontSet);
	return;
      }
      faceDesc->indexInFile = index;
      faceDesc->charSet = FcCharSetCreate();
      if (!faceDesc->charSet) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
	__glcFree(faceDesc->fileName);
	__glcFree(faceDesc->styleName);
	__glcFree(newNode);
	__glcFree(faceDesc);
	FcFontSetDestroy(fontSet);
	return;
      }
      faceDesc->charSet = FcCharSetUnion(faceDesc->charSet, charSet);

      newNode->data = faceDesc;

      if (inAppend)
	FT_List_Add(master->faceList, newNode);
      else
	FT_List_Insert(master->faceList, newNode);
    }
  }

  FcFontSetDestroy(fontSet);

  /* Append (or prepend) the directory name to the catalog list */
  __glcLock();
  if (inAppend) {
    if (!FcStrSetAddFilename(__glcCommonArea->catalogList,
			     (FcChar8*)inCatalog)) {
      __glcUnlock();
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }
  else {
    FcStrSet* newCatalogList = FcStrSetCreate();
    FcStrList* iterator = NULL;

    if (!FcStrSetAddFilename(newCatalogList, (FcChar8*)inCatalog)) {
      __glcUnlock();
      FcStrSetDestroy(newCatalogList);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
    iterator = FcStrListCreate(__glcCommonArea->catalogList);
    for (catalog = FcStrListNext(iterator); catalog;
	catalog = FcStrListNext(iterator)) {
      if (!FcStrSetAdd(newCatalogList, catalog)) {
	__glcUnlock();
	FcStrListDone(iterator);
	FcStrSetDestroy(newCatalogList);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }
    FcStrListDone(iterator);
    FcStrSetDestroy(__glcCommonArea->catalogList);
    __glcCommonArea->catalogList = newCatalogList;
  }
  __glcUnlock();
}



/* This function is called by glcRemoveCatalog. It has been included in the
 * context state class for the same reasons than addMasters was.
 */
void __glcCtxRemoveMasters(__glcContextState *This, GLint inIndex)
{
  int i = 0;
  FcConfig *config = FcConfigGetCurrent();
  FcFontSet *fontSet = NULL;
  FcStrSet *subDirs = NULL;
  FcStrList* iterator = NULL;
  FcChar8* catalog = NULL;

  __glcLock();
  iterator = FcStrListCreate(__glcCommonArea->catalogList);
  if (!iterator) {
    __glcUnlock();
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  for (catalog = FcStrListNext(iterator); catalog && inIndex;
	catalog = FcStrListNext(iterator), inIndex--);
  __glcUnlock();
  FcStrListDone(iterator);

  if (!catalog) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  fontSet = FcFontSetCreate();
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  
  subDirs = FcStrSetCreate();
  if (!subDirs) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!FcDirScan(fontSet, subDirs, NULL, FcConfigGetBlanks(config),
		 (const unsigned char*)catalog, FcFalse)) {
    FcFontSetDestroy(fontSet);
    FcStrSetDestroy(subDirs);
    return;
  }
  FcStrSetDestroy(subDirs); /* Sub directories are not scanned */

  for (i = 0; i < fontSet->nfont; i++) {
    FT_ListNode node = NULL;
    FcChar8 *fileName = NULL;

    /* get the file name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName)
	== FcResultTypeMismatch)
      continue;

    /* Search the file name of the font in the masters */
    for (node = This->masterList->head; node; node = node->next) {
      __glcMaster *master;
      FT_ListNode faceNode = NULL;
      FT_ListNode fontNode = NULL;

      master = (__glcMaster*)node->data;
      if (!master)
	continue;

      for (faceNode = master->faceList->head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = NULL;

	assert(faceNode->data);
	faceDesc = (__glcFaceDescriptor*)faceNode->data;
	if (!strcmp((const char*)faceDesc->fileName, (const char*)fileName))
	  break;
      }

      if (!faceNode)
	continue; /* The file is not in the current master, try the next one */

      /* Removes the corresponding faces in the font list */
      for (fontNode = This->fontList->head; fontNode; fontNode = fontNode->next) {
	__glcFont *font = (__glcFont*)fontNode->data;
	if (font) {
	  if (font->parent == master) {
	    /* Eventually remove the font from the current font list */
            FT_ListNode currentFontNode = NULL;
            for (currentFontNode = This->currentFontList->head; currentFontNode;
                 currentFontNode = currentFontNode->next) {
              if ((__glcFont*)currentFontNode->data == font) {
                FT_List_Remove(This->currentFontList, currentFontNode);
                __glcFree(currentFontNode);
		break;
	      }
	    }
	    glcDeleteFont(font->id);
	  }
	}
      }

      FT_List_Remove(master->faceList, faceNode);
      __glcFree(faceNode->data);
      __glcFree(faceNode);

      /* Remove characters of the font from the char list.
       * (Actually rebuild the master charset)
       */
      FcCharSetDestroy(master->charList);
      master->charList = FcCharSetCreate();
      for (faceNode = master->faceList->head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = NULL;

        assert(node->data);
	faceDesc = (__glcFaceDescriptor*)node->data;
	master->charList = FcCharSetUnion(master->charList, faceDesc->charSet);
      }
      master->minMappedCode = 0x7fffffff;
      master->maxMappedCode = 0;
      __glcUpdateCharList(master, master->charList);

      /* If the master is empty (i.e. does not contain any face) then
       * remove it.
       */
      if (!master->faceList->head) {
	__glcMasterDestroy(master);
	master = NULL;
      }
    }
  }

  FcFontSetDestroy(fontSet);
  __glcLock();
  FcStrSetDel(__glcCommonArea->catalogList, catalog);
  __glcUnlock();
  return;
}



/* Return the ID of the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'
 * If there is no such font, the function returns zero
 */
static GLint __glcLookupFont(GLint inCode, __glcContextState *inState)
{
  FT_ListNode node = NULL;

  for (node = inState->currentFontList->head; node; node = node->next) {
    const GLint font = ((__glcFont*)node->data)->id;
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
    FT_ListNode node = NULL;

    for (i = 0; i < glcGeti(GLC_FONT_COUNT); i++) {
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
    for (node = This->masterList->head; node; node = node->next) {
      if (glcGetMasterMap(((__glcMaster*)node->data)->id, inCode)) {
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



/* Since the common area can be accessed by any thread, this function should
 * be called before any access (read or write) to the common area. Otherwise
 * race conditons can occur.
 * __glcLock/__glcUnlock can be nested : they keep track of the number of
 * time they have been called and the mutex will be released as soon as
 * __glcUnlock() will be called as many time __glcLock().
 */
void __glcLock(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  if (!area->lockState)
    pthread_mutex_lock(&__glcCommonArea->mutex);

  area->lockState++;
}



/* Unlock the mutex in order to allow other threads to amke accesses to the
 * common area.
 * See also the note on nested calls in __glcLock's description.
 */
void __glcUnlock(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  area->lockState--;
  if (!area->lockState)
    pthread_mutex_unlock(&__glcCommonArea->mutex);
}



/* Sometimes informations may need to be stored temporarily by a thread.
 * The so-called 'buffer' is created for that purpose. Notice that it is a
 * component of the GLC state struct hence its lifetime is the same as the
 * GLC state's lifetime.
 * __glcCtxQueryBuffer() should be called whenever the buffer is to be used
 * in order to check if it is big enough to store infos.
 * Note that the only memory management function used below is 'realloc' which
 * means that the buffer goes bigger and bigger until it is freed. No function
 * is provided to reduce its size so it should be freed and re-allocated
 * manually in case of emergency ;-)
 */
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



/* Each thread has to store specific informations so they can retrieved later.
 * __glcGetThreadArea() returns a struct which contains thread specific info
 * for GLC. Notice that even if the lib does not support threads, this function
 * must be used.
 * If the 'threadArea' of the current thread does not exist, it is created and
 * initialized.
 * IMPORTANT NOTE : __glcGetThreadArea() must never use __glcMalloc() and
 *    __glcFree() since those functions could use the exceptContextStack
 *    before it is initialized.
 */
threadArea* __glcGetThreadArea(void)
{
  threadArea *area = NULL;

  area = (threadArea*)pthread_getspecific(__glcCommonArea->threadKey);

  if (!area) {
    area = (threadArea*)malloc(sizeof(threadArea));
    if (!area)
      return NULL;

    area->currentContext = NULL;
    area->errorState = GLC_NONE;
    area->lockState = 0;
    area->exceptContextStack = (FT_List) malloc(sizeof(FT_ListRec));
    if (!area->exceptContextStack) {
      free(area);
      return NULL;
    }
    area->exceptContextStack->head = NULL;
    area->exceptContextStack->tail = NULL;
    pthread_setspecific(__glcCommonArea->threadKey, (void*)area);
  }

  return area;
}
