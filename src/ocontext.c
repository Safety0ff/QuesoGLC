/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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
 * defines the object __GLCcontext which is used to manage the contexts.
 */

#if defined(__WIN32__) || defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "internal.h"
#include "texture.h"
#include FT_MODULE_H

__GLCcommonArea __glcCommonArea;
#ifdef HAVE_TLS
__thread __GLCthreadArea __glcTlsThreadArea;
#else
__GLCthreadArea* __glcThreadArea = NULL;
#endif

static void __glcContextUpdateHashTable(__GLCcontext *This);



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCcontext* __glcContextCreate(GLint inContext)
{
  __GLCcontext *This = NULL;

  This = (__GLCcontext*)__glcMalloc(sizeof(__GLCcontext));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCcontext));

  if (FT_New_Library(&__glcCommonArea.memoryManager, &This->library)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  FT_Add_Default_Modules(This->library);

#ifdef GLC_FT_CACHE
  if (FTC_Manager_New(This->library, 0, 0, 0, __glcFileOpen, NULL,
		      &This->cache)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
#endif

  This->config = FcInitLoadConfigAndFonts();
  if (!This->config) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;

  This->catalogList = __glcArrayCreate(sizeof(GLCchar8*));
  if (!This->catalogList) {
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->masterHashTable = __glcArrayCreate(sizeof(GLCchar32));
  if (!This->masterHashTable) {
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  __glcContextUpdateHashTable(This);

  This->currentFontList.head = NULL;
  This->currentFontList.tail = NULL;
  This->fontList.head = NULL;
  This->fontList.tail = NULL;
  This->genFontList.head = NULL;
  This->genFontList.tail = NULL;

  This->isCurrent = GL_FALSE;
  This->isInGlobalCommand = GL_FALSE;
  This->id = inContext;
  This->pendingDelete = GL_FALSE;
  This->stringState.callback = GLC_NONE;
  This->stringState.dataPointer = NULL;
  This->stringState.replacementCode = 0;
  This->stringState.stringType = GLC_UCS1;
  This->enableState.autoFont = GL_TRUE;
  This->enableState.glObjects = GL_TRUE;
  This->enableState.mipmap = GL_TRUE;
  This->enableState.hinting = GL_FALSE;
  This->enableState.extrude = GL_FALSE;
  This->enableState.kerning = GL_FALSE;
  This->renderState.resolution = 0.;
  This->renderState.renderStyle = GLC_BITMAP;
  This->bitmapMatrixStackDepth = 1;
  This->bitmapMatrix = This->bitmapMatrixStack;
  This->bitmapMatrix[0] = 1.;
  This->bitmapMatrix[1] = 0.;
  This->bitmapMatrix[2] = 0.;
  This->bitmapMatrix[3] = 1.;
  This->attribStackDepth = 0;
  This->measurementBuffer = __glcArrayCreate(12 * sizeof(GLfloat));
  if (!This->measurementBuffer) {
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->isInCallbackFunc = GL_FALSE;
  This->buffer = NULL;
  This->bufferSize = 0;
  This->vertexArray = __glcArrayCreate(2 * sizeof(GLfloat));
  if (!This->vertexArray) {
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->controlPoints = __glcArrayCreate(5 * sizeof(GLfloat));
  if (!This->controlPoints) {
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }
  This->endContour = __glcArrayCreate(sizeof(int));
  if (!This->endContour) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->vertexIndices = __glcArrayCreate(sizeof(GLuint));
  if (!This->vertexIndices) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->geomBatches = __glcArrayCreate(sizeof(__GLCgeomBatch));
  if (!This->geomBatches) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcArrayDestroy(This->masterHashTable);
    __glcArrayDestroy(This->endContour);
    __glcArrayDestroy(This->vertexIndices);
    __glcArrayDestroy(This->catalogList);
#ifdef GLC_FT_CACHE
    FTC_Manager_Done(This->cache);
#endif
    FT_Done_Library(This->library);
    FcConfigDestroy(This->config);
    __glcFree(This);
    return NULL;
  }

  This->texture.id = 0;
  This->texture.width = 0;
  This->texture.heigth = 0;
  This->texture.bufferObjectID = 0;

  This->atlas.id = 0;
  This->atlas.width = 0;
  This->atlas.heigth = 0;
  This->atlasList.head = NULL;
  This->atlasList.tail = NULL;
  This->atlasWidth = 0;
  This->atlasHeight = 0;
  This->atlasCount = 0;

  /* The environment variable GLC_PATH is an alternate way to allow QuesoGLC
   * to access to fonts catalogs/directories.
   */
  /*Check if the GLC_PATH environment variables are exported */
  if (getenv("GLC_CATALOG_LIST") || getenv("GLC_PATH")) {
    char *path = NULL;
    char *begin = NULL;
    char *sepPos = NULL;
    char *separator = getenv("GLC_LIST_SEPARATOR");

    /* Get the list separator */
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

    /* Read the paths of fonts file.
     * First, try GLC_CATALOG_LIST...
     */
    if (getenv("GLC_CATALOG_LIST"))
#ifdef __WIN32__
      path = _strdup(getenv("GLC_CATALOG_LIST"));
#else
      path = strdup(getenv("GLC_CATALOG_LIST"));
#endif
    else if (getenv("GLC_PATH")) {
      /* Try GLC_PATH which uses the same format than PATH */
#ifdef __WIN32__
      path = _strdup(getenv("GLC_PATH"));
#else
      path = strdup(getenv("GLC_PATH"));
#endif
    }

    if (path) {
      /* Get each path and add the corresponding masters to the current
       * context */
      GLCchar8* dup = NULL;

      begin = path;
      do {
	sepPos = (char *)__glcFindIndexList(begin, 1, separator);

        if (*sepPos)
	  *(sepPos++) = 0;

#ifdef __WIN32__
        dup = (GLCchar8*)_strdup(begin);
#else
        dup = (GLCchar8*)strdup(begin);
#endif
        if (!dup) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
        }
        else {
	  if (!__glcArrayAppend(This->catalogList, &dup))
            free(dup);
          else if (!FcConfigAppFontAddDir(This->config,
                                          (const unsigned char*)begin)) {
            __glcArrayRemove(This->catalogList,
                             GLC_ARRAY_LENGTH(This->catalogList));
	    __glcRaiseError(GLC_RESOURCE_ERROR);
            free(dup);
          }
        }

        begin = sepPos;
      } while (*sepPos);
      free(path);
      __glcContextUpdateHashTable(This);
    }
    else {
      /* strdup has failed to allocate memory to duplicate GLC_PATH => ERROR */
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
  }

  return This;
}



#ifndef GLC_FT_CACHE
/* This function is called from FT_List_Finalize() to close all the fonts
 * of the GLC_CURRENT_FONT_LIST
 */
static void __glcFontClosure(FT_Memory GLC_UNUSED_ARG(inMemory), void* inData,
			     void* GLC_UNUSED_ARG(inUser))
{
  __GLCfont *font = (__GLCfont*)inData;

  assert(font);
  __glcFontClose(font);
}
#endif



/* This function is called from FT_List_Finalize() to destroy all
 * remaining fonts
 */
static void __glcFontDestructor(FT_Memory GLC_UNUSED_ARG(inMemory),
                                void* inData, void* inUser)
{
  __GLCfont *font = (__GLCfont*)inData;
  __GLCcontext* ctx = (__GLCcontext*)inUser;

  assert(ctx);
  assert(font);
  __glcFontDestroy(font, ctx);
}



/* Destructor of the object : it first destroys all the GLC objects that have
 * been created during the life of the context. Then it releases the memory
 * occupied by the GLC state struct.
 * It does not destroy GL objects associated with the GLC context since we can
 * not be sure that the current GL context is the GL context that contains the
 * GL objects designated by the GLC context that we are destroying. This could
 * happen if the user calls glcDeleteContext() after the GL context has been
 * destroyed of after the user has changed the current GL context.
 */
void __glcContextDestroy(__GLCcontext *This)
{
  int i = 0;

  assert(This);

  /* Destroy the list of catalogs */
  for (i = 0; i < GLC_ARRAY_LENGTH(This->catalogList); i++) {
    GLCchar8* string = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[i];

    assert(string);
    free(string);
  }
  __glcArrayDestroy(This->catalogList);

  /* Destroy GLC_CURRENT_FONT_LIST */
#ifdef GLC_FT_CACHE
  FT_List_Finalize(&This->currentFontList, NULL,
		   &__glcCommonArea.memoryManager, NULL);
#else
  FT_List_Finalize(&This->currentFontList, __glcFontClosure,
		   &__glcCommonArea.memoryManager, NULL);
#endif

  /* Destroy GLC_FONT_LIST */
  FT_List_Finalize(&This->fontList, __glcFontDestructor,
                   &__glcCommonArea.memoryManager, This);
  /* Destroy empty fonts generated by glcGenFontID() */
  FT_List_Finalize(&This->genFontList, __glcFontDestructor,
		   &__glcCommonArea.memoryManager, This);

  if (This->masterHashTable)
    __glcArrayDestroy(This->masterHashTable);

  FT_List_Finalize(&This->atlasList, NULL,
		   &__glcCommonArea.memoryManager, NULL);

  if (This->bufferSize)
    __glcFree(This->buffer);

  if (This->measurementBuffer)
    __glcArrayDestroy(This->measurementBuffer);

  if (This->vertexArray)
    __glcArrayDestroy(This->vertexArray);

  if (This->controlPoints)
    __glcArrayDestroy(This->controlPoints);

  if (This->endContour)
    __glcArrayDestroy(This->endContour);

  if (This->vertexIndices)
    __glcArrayDestroy(This->vertexIndices);

  if (This->geomBatches)
    __glcArrayDestroy(This->geomBatches);

#ifdef GLC_FT_CACHE
  FTC_Manager_Done(This->cache);
#endif
  FT_Done_Library(This->library);
  FcConfigDestroy(This->config);
  __glcFree(This);
}



/* Return the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'.
 * If there is no such font, the function returns NULL.
 * 'inCode' must be given in UCS-4 format.
 */
static __GLCfont* __glcLookupFont(GLint inCode, FT_List fontList)
{
  FT_ListNode node = NULL;

  for (node = fontList->head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)node->data;

    /* Check if the character identified by inCode exists in the font */
    if (__glcCharMapHasChar(font->charMap, inCode))
      return font;
  }
  return NULL;
}



/* Calls the callback function (does various tests to determine if it is
 * possible) and returns GL_TRUE if it has succeeded or GL_FALSE otherwise.
 * 'inCode' must be given in UCS-4 format.
 */
static GLboolean __glcCallCallbackFunc(GLint inCode,
				       __GLCcontext *inContext)
{
  GLCfunc callbackFunc = NULL;
  GLboolean result = GL_FALSE;
  GLint aCode = 0;

  /* Recursivity is not allowed */
  if (inContext->isInCallbackFunc)
    return GL_FALSE;

  callbackFunc = inContext->stringState.callback;
  if (!callbackFunc)
    return GL_FALSE;

  /* Convert the character code back to the current string type */
  aCode = __glcConvertUcs4ToGLint(inContext, inCode);
  /* Check if the character has been converted */
  if (aCode < 0)
    return GL_FALSE;

  inContext->isInCallbackFunc = GL_TRUE;
  /* Call the callback function with the character converted to the current
   * string type.
   */
  result = (*callbackFunc)(aCode);
  inContext->isInCallbackFunc = GL_FALSE;

  return result;
}



/* Returns the ID of the first font in GLC_CURRENT_FONT_LIST that maps
 * 'inCode'. If there is no such font and GLC_AUTO_FONT is enabled, the
 * function attempts to append a new font from GLC_FONT_LIST (or from a master)
 * to GLC_CURRENT_FONT_LIST. If the attempt fails the function returns zero.
 * 'inCode' must be given in UCS-4 format.
 */
__GLCfont* __glcContextGetFont(__GLCcontext *This, GLint inCode)
{
  __GLCfont* font = NULL;

  /* Look for a font in the current font list */
  font = __glcLookupFont(inCode, &This->currentFontList);
  /* If a font has been found return */
  if (font)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(inCode, This)) {
    font = __glcLookupFont(inCode, &This->currentFontList);
    if (font)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (This->enableState.autoFont) {
    __GLCmaster* master = NULL;

    font = __glcLookupFont(inCode, &This->fontList);
    if (font) {
      __glcAppendFont(This, font);
      return font;
    }

    master = __glcMasterMatchCode(This, inCode);
    if (!master)
      return NULL;
    font = __glcNewFontFromMaster(glcGenFontID(), master, This, inCode);
    __glcMasterDestroy(master);

    if (font) {
      /* Add the font to the GLC_CURRENT_FONT_LIST */
      __glcAppendFont(This, font);
      return font;
    }
  }
  return NULL;
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
GLCchar* __glcContextQueryBuffer(__GLCcontext *This, int inSize)
{
  GLCchar* buffer;

  buffer = This->buffer;
  if (inSize > This->bufferSize) {
    buffer = (GLCchar*)__glcRealloc(This->buffer, inSize);
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
    }
    else {
      This->buffer = buffer;
      This->bufferSize = inSize;
    }
  }

  return buffer;
}



/* Update the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
static void __glcContextUpdateHashTable(__GLCcontext *This)
{
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;

  /* Use Fontconfig to get the default font files */
  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return;
  }
  fontSet = FcFontList(This->config, pattern, objectSet);
  FcPatternDestroy(pattern);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Parse the font set looking for fonts that are not already registered in the
   * hash table.
   */
  for (i = 0; i < fontSet->nfont; i++) {
    GLCchar32 hashValue = 0;
    int j = 0;
    int length = GLC_ARRAY_LENGTH(This->masterHashTable);
    GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(This->masterHashTable);
    FcBool outline = FcFalse;
    FcChar8* family = NULL;
    int fixed = 0;
    FcChar8* foundry = NULL;
    FcResult result = FcResultMatch;

    /* Check whether the glyphs are outlines */
    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (!outline)
      continue;

    result = FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &family);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &foundry);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
    assert(result != FcResultTypeMismatch);

    if (foundry)
      pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
			       FC_FOUNDRY, FcTypeString, foundry, FC_SPACING,
			       FcTypeInteger, fixed, NULL);
    else
      pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
			       FC_SPACING, FcTypeInteger, fixed, NULL);

    if (!pattern) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcFontSetDestroy(fontSet);
      return;
    }

    /* Check if the font is already registered in the hash table */
    hashValue = FcPatternHash(pattern);
    FcPatternDestroy(pattern);
    for (j = 0; j < length; j++) {
      if (hashTable[j] == hashValue)
	break;
    }

    /* If the font is already registered then parse the next one */
    if (j != length)
      continue;

    /* Register the font (i.e. append its hash value to the hash table) */
    if (!__glcArrayAppend(This->masterHashTable, &hashValue)) {
      FcFontSetDestroy(fontSet);
      return;
    }
  }

  FcFontSetDestroy(fontSet);
}



/* Append a catalog to the context catalog list */
void __glcContextAppendCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
#ifdef __WIN32__
  GLCchar8* dup = (GLCchar8*)_strdup(inCatalog);
#else
  GLCchar8* dup = (GLCchar8*)strdup(inCatalog);
#endif

  if (!dup) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!__glcArrayAppend(This->catalogList, &dup)) {
    free(dup);
    return;
  }

  if (!FcConfigAppFontAddDir(This->config, (const unsigned char*)inCatalog)) {
    __glcArrayRemove(This->catalogList, GLC_ARRAY_LENGTH(This->catalogList));
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(dup);
    return;
  }
  __glcContextUpdateHashTable(This);
}



/* Prepend a catalog to the context catalog list */
void __glcContextPrependCatalog(__GLCcontext* This, const GLCchar* inCatalog)
{
#ifdef __WIN32__
  GLCchar8* dup = (GLCchar8*)_strdup(inCatalog);
#else
  GLCchar8* dup = (GLCchar8*)strdup(inCatalog);
#endif

  if (!dup) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!__glcArrayInsert(This->catalogList, 0, &dup)) {
    free(dup);
    return;
  }

  if (!FcConfigAppFontAddDir(This->config, (const unsigned char*)inCatalog)) {
    __glcArrayRemove(This->catalogList, 0);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(dup);
    return;
  }
  __glcContextUpdateHashTable(This);
}



/* Get the path of the catalog identified by inIndex */
GLCchar8* __glcContextGetCatalogPath(__GLCcontext* This, GLint inIndex)
{
  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }
    
  return ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[inIndex];
}



/* Remove a catalog from the context catalog list */
void __glcContextRemoveCatalog(__GLCcontext* This, GLint inIndex)
{
  FT_ListNode node = NULL;
  GLCchar8* catalog = NULL;
  int i = 0;

  if (inIndex >= GLC_ARRAY_LENGTH(This->catalogList)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  FcConfigAppFontClear(This->config);
  catalog = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[inIndex];
  assert(catalog);
  __glcArrayRemove(This->catalogList, inIndex);
  free(catalog);

  for (i = 0; i < GLC_ARRAY_LENGTH(This->catalogList); i++) {
    catalog = ((GLCchar8**)GLC_ARRAY_DATA(This->catalogList))[i];
    assert(catalog);
    if (!FcConfigAppFontAddDir(This->config, catalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcArrayRemove(This->catalogList, i);
      free(catalog);
      if (i > 0) i--;
    }
  }

  /* Re-create the hash table from scratch */
  GLC_ARRAY_LENGTH(This->masterHashTable) = 0;
  __glcContextUpdateHashTable(This);

  /* Remove from GLC_FONT_LIST the fonts that were defined in the catalog that
   * has been removed.
   */
  for (node = This->fontList.head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)(node->data);
    GLCchar32 hashValue = 0;
    GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(This->masterHashTable);
    int length = GLC_ARRAY_LENGTH(This->masterHashTable);
    int i = 0;
    __GLCmaster* master = __glcMasterCreate(font->parentMasterID, This);

    if (!master)
      continue;

    /* Check if the hash value of the master is in the hash table */
    hashValue = GLC_MASTER_HASH_VALUE(master);
    for (i = 0; i < length; i++) {
      if (hashValue == hashTable[i])
	break;
    }

    /* The font is not contained in the hash table => remove it */
    if (i == length)
      glcDeleteFont(font->id);

    __glcMasterDestroy(master);
  }
}
