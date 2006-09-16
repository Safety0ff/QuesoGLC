/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2006, Bertrand Coconnier
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

#include <sys/stat.h>
#include <unistd.h>

#include "internal.h"
#include "texture.h"
#include FT_MODULE_H
#include FT_LIST_H

commonArea __glcCommonArea;

/** \file
 * defines the object __glcContext which is used to manage the contexts.
 */

/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
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

  if (FT_New_Library(&__glcCommonArea.memoryManager, &This->library)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  FT_Add_Default_Modules(This->library);

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;

  This->masterList.head = NULL;
  This->masterList.tail = NULL;
  This->catalogList = FcStrSetCreate();
  if (!This->catalogList) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    __glcFree(This);
    return NULL;
  }
  This->currentFontList.head = NULL;
  This->currentFontList.tail = NULL;
  This->fontList.head = NULL;
  This->fontList.tail = NULL;

  This->isCurrent = GL_FALSE;
  This->id = inContext;
  This->pendingDelete = GL_FALSE;
  This->callback = GLC_NONE;
  This->dataPointer = NULL;
  This->autoFont = GL_TRUE;
  This->glObjects = GL_TRUE;
  This->hinting = GL_FALSE;
  This->mipmap = GL_TRUE;
  This->resolution = 0.;
  This->bitmapMatrix[0] = 1.;
  This->bitmapMatrix[1] = 0.;
  This->bitmapMatrix[2] = 0.;
  This->bitmapMatrix[3] = 1.;
  This->measurementBuffer = __glcArrayCreate(12 * sizeof(GLfloat));
  if (!This->measurementBuffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    FcStrSetDestroy(This->catalogList);
    __glcFree(This);
    return NULL;
  }
  This->renderStyle = GLC_BITMAP;
  This->replacementCode = 0;
  This->stringType = GLC_UCS1;
  This->isInCallbackFunc = GL_FALSE;
  This->buffer = NULL;
  This->bufferSize = 0;
  This->lastFontID = 1;
  This->vertexArray = __glcArrayCreate(2 * sizeof(GLfloat));
  if (!This->vertexArray) {
    __glcArrayDestroy(This->measurementBuffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    FcStrSetDestroy(This->catalogList);
    __glcFree(This);
    return NULL;
  }
  This->controlPoints = __glcArrayCreate(7 * sizeof(GLfloat));
  if (!This->vertexArray) {
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    FcStrSetDestroy(This->catalogList);
    __glcFree(This);
    return NULL;
  }
  This->endContour = __glcArrayCreate(sizeof(int));
  if (!This->endContour) {
    __glcArrayDestroy(This->controlPoints);
    __glcArrayDestroy(This->vertexArray);
    __glcArrayDestroy(This->measurementBuffer);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FT_Done_Library(This->library);
    FcStrSetDestroy(This->catalogList);
    __glcFree(This);
    return NULL;
  }

  This->glCapacities = 0;
  This->texture.id = 0;
  This->texture.width = 0;
  This->texture.heigth = 0;

  This->atlas.id = 0;
  This->atlas.width = 0;
  This->atlas.heigth = 0;
  This->atlasList.head = NULL;
  This->atlasList.tail = NULL;
  This->atlasWidth = 0;
  This->atlasHeight = 0;
  This->atlasCount = 0;

  return This;
}



/* This function is called from FT_List_Finalize() to destroy all
 * remaining fonts
 */
static void __glcFontDestructor(FT_Memory inMemory, void* inData, void* inUser)
{
  __glcFont *font = (__glcFont*)inData;

  if (font)
    __glcFontDestroy(font);
}



/* This function is called from FT_List_Finalize() to destroy all
 * atlas elements and update the glyphs accordingly
 */
static void __glcAtlasDestructor(FT_Memory inMemory, void* inData, void* inUser)
{
  __glcAtlasElement* element = (__glcAtlasElement*)inData;
  __glcGlyph* glyph = NULL;

  assert(element);

  glyph = element->glyph;
  if (glyph)
    __glcGlyphDestroyTexture(glyph);
}



/* Destructor of the object : it first destroys all the objects (whether they
 * are internal GLC objects or GL textures or GL display lists) that have been
 * created during the life of the context. Then it releases the memory occupied
 * by the GLC state struct.
 */
void __glcCtxDestroy(__glcContextState *This)
{
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

  assert(This);

  /* Destroy GLC_CURRENT_FONT_LIST */
  FT_List_Finalize(&This->currentFontList, NULL,
		   &__glcCommonArea.memoryManager, NULL);

  /* Destroy GLC_FONT_LIST */
  FT_List_Finalize(&This->fontList, __glcFontDestructor,
                   &__glcCommonArea.memoryManager, NULL);

  /* Destroy the list of catalogs */
  FcStrSetDestroy(This->catalogList);

  /* destroy remaining masters */
  node = This->masterList.head;
  while (node) {
    next = node->next;
    __glcMasterDestroy((__glcMaster*)node);
    node = next;
  }

  if (This->texture.id)
    glDeleteTextures(1, &This->texture.id);

  if (This->atlas.id)
    glDeleteTextures(1, &This->atlas.id);

  FT_List_Finalize(&This->atlasList, __glcAtlasDestructor,
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

  FT_Done_Library(This->library);
  __glcFree(This);
}



/* This function parses a font set and add the font files to the masters
 * of the context 'This'. Masters are created if necessary.
 */
void __glcAddFontsToContext(__glcContextState *This, FcFontSet *fontSet,
			    GLboolean inAppend)
{
  int i = 0;

  /* For each font of the font set */
  for (i = 0; i < fontSet->nfont; i++) {
    __glcMaster *master = NULL;
    FcChar8 *fileName = NULL;
    FcChar8 *vendorName = NULL;
    FcChar8 *familyName = NULL;
    FcChar8 *styleName = NULL;
    FT_ListNode node = NULL;
    FcBool outline = FcFalse;
    int fixed = 0;
    FcCharSet *charSet = NULL;
    int index = 0;

    /* Check whether the glyphs are outlines */
    if (FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline)
	== FcResultTypeMismatch)
      continue;
    if (!outline)
      continue;

    /* get the file name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName)
	== FcResultTypeMismatch)
      continue;

    /* get the vendor name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &vendorName)
	== FcResultTypeMismatch)
      vendorName = NULL;

    /* get the style name */
    if (FcPatternGetString(fontSet->fonts[i], FC_STYLE, 0, &styleName)
	== FcResultTypeMismatch)
      continue;

    /* get the family name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &familyName)
	== FcResultTypeMismatch)
      continue;

    /* Is this a fixed font ? */
    if (FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed)
	== FcResultTypeMismatch)
      continue;

    /* get the char set */
    if (FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet)
	== FcResultTypeMismatch)
      continue;
    else {
      /* Check that the char set is not empty */
      FcChar32 base = 0;
      FcChar32 next = 0;
      FcChar32 map[FC_CHARSET_MAP_SIZE];

      base = FcCharSetFirstPage(charSet, map, &next);
      if (base == FC_CHARSET_DONE)
        continue;
    }

    /* get the index of the font in font file */
    if (FcPatternGetInteger(fontSet->fonts[i], FC_INDEX, 0, &index)
	== FcResultTypeMismatch)
      continue;

    /* Determine if the family (i.e. "Times", "Courier", ...) is already
     * associated to a master.
     */
    for (node = This->masterList.head; node; node = node->next) {
      master = (__glcMaster*)node;

      if (!strcmp((const char*)familyName, (const char*)master->family))
	break;
    }

    if (!node) {
      /* No master has been found. We must create one */
      GLint id;

      /* Get the master ID */
      if (master)
	/* master is already equal to the last master of masterList
	 * since the master list has been parsed in the "for" loop
	 * and no master has been found.
	 */
	id = master->id + 1;
      else
	id = 0;

      /* Create a new master and add it to the current context */
      master = __glcMasterCreate(familyName, vendorName, id, This->stringType);
      if (!master) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	break;
      }

      FT_List_Add(&This->masterList, (FT_ListNode)master);
    }

    /* Parse the faces of the master to check that does not already exists */
    for (node = master->faceList.head; node; node = node->next) {
      __glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)node;

      if (!strcmp((const char*)faceDesc->styleName, (const char*)styleName))
	break;
    }

    if (!node) {
      /* The current face in the font file is not already loaded in a
       * master : Append (or prepend) the new face and its file name to
       * the master.
       */
      __glcFaceDescriptor* faceDesc = NULL;
      FT_ListNode newNode = NULL;
      __glcCharMap* charMap = __glcCharMapCreate(charSet);

      if (!charMap) {
	FcFontSetDestroy(fontSet);
	return;
      }

      /* Merge the character map of the new face with the character map of the
       * master.
       */
      if (!__glcCharMapUnion(master->charList, charMap)) {
	FcFontSetDestroy(fontSet);
	__glcCharMapDestroy(charMap);
	return;
      }

      __glcCharMapDestroy(charMap);

      /* Create a face descriptor that will contain the whole description of
       * the face (hence the name).
       */
      faceDesc = __glcFaceDescCreate(styleName, charSet,
				     (fixed != FC_PROPORTIONAL), fileName,
				     index);
      if (!faceDesc) {
	FcFontSetDestroy(fontSet);
	return;
      }

      /* Append or prepend (depending on inAppend) the new face to the
       * master.
       */
      newNode = (FT_ListNode)faceDesc;
      newNode->data = faceDesc;

      if (inAppend)
	FT_List_Add(&master->faceList, newNode);
      else
	FT_List_Insert(&master->faceList, newNode);
    }
  }
}



/* This function is called by glcAppendCatalog and glcPrependCatalog. It has
 * been associated to the contextState class rather than the master class
 * because glcAppendCatalog and glcPrependCatalog might create several masters.
 * Moreover it associates the new masters (if any) to the current context.
 */
void __glcCtxAddMasters(__glcContextState *This, const GLCchar* inCatalog,
			GLboolean inAppend)
{
  FcConfig *config = FcConfigGetCurrent();
  struct stat dirStat;
  FcFontSet *fontSet = NULL;
  FcStrSet *subDirs = NULL;
  FcChar8* catalog = NULL;

  /* Verify that the catalog has not already been appended (or prepended) */
  if (FcStrSetMember(This->catalogList, (FcChar8*)inCatalog))
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
  /* Check that 'path' is a directory */
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

  /* A Fontconfig object that will contain the result of directory scan */
  fontSet = FcFontSetCreate();
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }

  /* A Fontconfig object that will contain the subdirectories of 'path' */
  subDirs = FcStrSetCreate();
  if (!subDirs) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(catalog);
    return;
  }

  /* Perform the scanning of the directory */
  if (!FcDirScan(fontSet, subDirs, NULL, FcConfigGetBlanks(config),
		 (const unsigned char*)catalog, FcFalse)) {
    FcFontSetDestroy(fontSet);
    FcStrSetDestroy(subDirs);
    __glcFree(catalog);
    return;
  }
  FcStrSetDestroy(subDirs); /* Sub directories are ignored */
  __glcFree(catalog);

  /* Add the fonts found in the directory to the masters of the current
   * context.
   */
  __glcAddFontsToContext(This, fontSet, inAppend);

  FcFontSetDestroy(fontSet);

  /* Append (or prepend) the directory name to the catalog list */
  if (inAppend) {
    if (!FcStrSetAddFilename(This->catalogList, (FcChar8*)inCatalog)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }
  else {
    /* In order to prepend the directory name we must create a new catalog
     * list with the new directory first then all the catalogs of the previous
     * list.
     */
    FcStrSet* newCatalogList = FcStrSetCreate();
    FcStrList* iterator = NULL;

    /* Add the new catalog first */
    if (!FcStrSetAddFilename(newCatalogList, (FcChar8*)inCatalog)) {
      FcStrSetDestroy(newCatalogList);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
    /* Iterate over the catalogs stored in the previous list */
    iterator = FcStrListCreate(This->catalogList);
    /* TODO : what if iterator can not be allocated ? Arrghh!!! Then we can not
     * get back to the previous state of the context since the new masters and
     * the new faces have already been added.
     * This should be fixed one day...
     */
    for (catalog = FcStrListNext(iterator); catalog;
	catalog = FcStrListNext(iterator)) {
      /* Add the current catalog to the new list */
      if (!FcStrSetAdd(newCatalogList, catalog)) {
	FcStrListDone(iterator);
	FcStrSetDestroy(newCatalogList);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }
    }
    /* Free memory */
    FcStrListDone(iterator);
    FcStrSetDestroy(This->catalogList);
    This->catalogList = newCatalogList;
  }
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

  /* Look for the catalog to be removed */
  iterator = FcStrListCreate(This->catalogList);
  if (!iterator) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  for (catalog = FcStrListNext(iterator); catalog && inIndex;
	catalog = FcStrListNext(iterator), inIndex--);
  FcStrListDone(iterator);

  /* The catalog has not been found : raise an error and return */
  if (!catalog) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* A Fontconfig object that will contain the result of directory scan */
  fontSet = FcFontSetCreate();
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* A Fontconfig object that will contain the subdirectories of the catalog */
  subDirs = FcStrSetCreate();
  if (!subDirs) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Perform the scanning of the directory */
  if (!FcDirScan(fontSet, subDirs, NULL, FcConfigGetBlanks(config),
		 (const unsigned char*)catalog, FcFalse)) {
    FcFontSetDestroy(fontSet);
    FcStrSetDestroy(subDirs);
    return;
  }
  FcStrSetDestroy(subDirs); /* Sub directories are not scanned */

  /* For each font that has been found in the catalog */
  for (i = 0; i < fontSet->nfont; i++) {
    FT_ListNode node = NULL;
    FT_ListNode masterNode = NULL;
    FT_ListNode faceNode = NULL;
    FcChar8 *fileName = NULL;
    __glcMaster *master = NULL;
    __glcFaceDescriptor* faceDesc = NULL;
    __glcFont* font = NULL;

    /* get the file name */
    if (FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName)
	== FcResultTypeMismatch)
      continue;

    /* Search the corresponding face in the masters */
    for (masterNode = This->masterList.head; masterNode;
	 masterNode = masterNode->next) {

      master = (__glcMaster*)masterNode;
      assert(master);

      /* Search the file name of the font in the faces of the current master */
      for (faceNode = master->faceList.head; faceNode;
	   faceNode = faceNode->next) {
	faceDesc = (__glcFaceDescriptor*)faceNode;

	if (!strcmp((const char*)faceDesc->fileName, (const char*)fileName))
	  break;
      }

      /* The file name has been found exit */
      if (faceNode)
	break;
    }

    if (!masterNode)
      continue; /* The file is not in a master, it may have been added since
		 * QuesoGLC has been launched !?! */

    /* Look if the corresponding face is stored in the font list */
    for (node = This->fontList.head; node; node = node->next) {
      font = (__glcFont*)node->data;
      assert(font);
      if (font->faceDesc == faceDesc)
	break;
    }

    if(node) {
      /*A font with the corresponding face descriptor has been found */
      assert(font->parent == master);

      /* Eventually remove the font from the current font list */
      for (node = This->currentFontList.head; node; node = node->next) {
	if ((__glcFont*)node->data == font) {
	  FT_List_Remove(&This->currentFontList, node);
	  __glcFree(node);
	  break;
	}
      }

      glcDeleteFont(font->id);
    }

    /* Delete the face descriptor */
    FT_List_Remove(&master->faceList, (FT_ListNode)faceDesc);
    __glcFaceDescDestroy(faceDesc);

    /* If the master is empty (i.e. does not contain any face) then
     * remove it.
     */
    if (!master->faceList.head) {
      FT_List_Remove(&This->masterList, masterNode);
      __glcMasterDestroy(master);
      master = NULL;
    }
    else {
      /* Remove characters of the font from the char list.
       * (Actually rebuild the master charset)
       */
      __glcCharMapDestroy(master->charList);
      master->charList = __glcCharMapCreate(NULL);

      for (faceNode = master->faceList.head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)faceNode;
	__glcCharMap* charMap = __glcFaceDescGetCharMap(faceDesc, This);

	if (!charMap) {
	  FcFontSetDestroy(fontSet);
	  FcStrSetDel(This->catalogList, catalog);
	  return;
	}

	/* TODO: handle the case of failure of __glcCharMapUnion and to
	 * be able to restore the master in a coherent state
	 */
	__glcCharMapUnion(master->charList, charMap);
	__glcCharMapDestroy(charMap);
      }
    }
  }

  FcFontSetDestroy(fontSet);
  FcStrSetDel(This->catalogList, catalog);
  return;
}



/* Return the ID of the first font in GLC_CURRENT_FONT_LIST that maps 'inCode'
 * If there is no such font, the function returns zero.
 * 'inCode' must be given in UCS-4 format.
 */
static GLint __glcLookupFont(GLint inCode, FT_List fontList)
{
  FT_ListNode node = NULL;

  for (node = fontList->head; node; node = node->next) {
    __glcFont* font = (__glcFont*)node->data;

    /* Check if the character identified by inCode exists in the font */
    if (__glcCharMapHasChar(font->charMap, inCode))
      return font->id;
  }
  return -1;
}



/* Calls the callback function (does various tests to determine if it is
 * possible) and returns GL_TRUE if it has succeeded or GL_FALSE otherwise.
 * 'inCode' must be given in UCS-4 format.
 */
static GLboolean __glcCallCallbackFunc(GLint inCode,
				       __glcContextState *inState)
{
  GLCfunc callbackFunc = NULL;
  GLboolean result = GL_FALSE;
  GLint aCode = 0;

  /* Recursivity is not allowed */
  if (inState->isInCallbackFunc)
    return GL_FALSE;

  callbackFunc = inState->callback;
  if (!callbackFunc)
    return GL_FALSE;

  /* Convert the character code back to the current string type */
  aCode = __glcConvertUcs4ToGLint(inState, inCode);
  /* Check if the character has been converted */
  if (aCode < 0)
    return GL_FALSE;

  inState->isInCallbackFunc = GL_TRUE;
  /* Call the callback function with the character converted to the current
   * string type.
   */
  result = (*callbackFunc)(aCode);
  inState->isInCallbackFunc = GL_FALSE;

  return result;
}



/* Returns the ID of the first font in GLC_CURRENT_FONT_LIST that maps
 * 'inCode'. If there is no such font and GLC_AUTO_FONT is enabled, the
 * function attempts to append a new font from GLC_FONT_LIST (or from a master)
 * to GLC_CURRENT_FONT_LIST. If the attempt fails the function returns zero.
 * 'inCode' must be given in UCS-4 format.
 */
GLint __glcCtxGetFont(__glcContextState *This, GLint inCode)
{
  GLint font = 0;

  /* Look for a font in the current font list */
  font = __glcLookupFont(inCode, &This->currentFontList);
  /* If a font has been found return */
  if (font >= 0)
    return font;

  /* If a callback function is defined for GLC_OP_glcUnmappedCode then call it.
   * The callback function should return GL_TRUE if it succeeds in appending to
   * GLC_CURRENT_FONT_LIST the ID of a font that maps 'inCode'.
   */
  if (__glcCallCallbackFunc(inCode, This)) {
    font = __glcLookupFont(inCode, &This->currentFontList);
    if (font >= 0)
      return font;
  }

  /* If the value of the boolean variable GLC_AUTOFONT is GL_TRUE then search
   * GLC_FONT_LIST for the first font that maps 'inCode'. If the search
   * succeeds, then append the font's ID to GLC_CURRENT_FONT_LIST.
   */
  if (This->autoFont) {
    FT_ListNode node = NULL;

    font = __glcLookupFont(inCode, &This->fontList);
    if (font >= 0) {
      glcAppendFont(font);
      return font;
    }

    /* Otherwise, the function searches the GLC master list for the first
     * master that maps 'inCode'. If the search succeeds, it creates a font
     * from the master and appends its ID to GLC_CURRENT_FONT_LIST.
     */
    for (node = This->masterList.head; node; node = node->next) {
      __glcMaster* master = (__glcMaster*)node;
      FT_ListNode faceNode = NULL;

      /* Check if the corresponding character is stored in the master */
      if (!__glcCharMapHasChar(master->charList, (FcChar32)inCode))
        continue; /* Failed : try the next master */

      /* We search for a font file that supports the requested inCode glyph */
      for (faceNode = master->faceList.head; faceNode;
	   faceNode = faceNode->next) {
	__glcFaceDescriptor* faceDesc = (__glcFaceDescriptor*)faceNode;
	__glcCharMap* charMap = __glcFaceDescGetCharMap(faceDesc, This);

	if (!charMap)
	  continue;

	/* Check if the face descriptor contains the requested character */
	if (__glcCharMapHasChar(charMap, (FcChar32)inCode)) {
	  /* Create a new font from the face descriptor */
	  font = glcNewFontFromMaster(glcGenFontID(), master->id);
	  if (font) {
	    __glcCharMapDestroy(charMap);
	    /* Add the font to the GLC_CURRENT_FONT_LIST */
	    glcAppendFont(font);
	    glcFontFace(font, faceDesc->styleName);
	    return font;
	  }
	}
 	__glcCharMapDestroy(charMap);
      }
    }
  }
  return -1;
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
GLCchar* __glcCtxQueryBuffer(__glcContextState *This, int inSize)
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
