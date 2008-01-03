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
 * defines the object __GLCmaster which manage the masters
 */

#include "internal.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCmaster* __glcMasterCreate(GLint inMaster, __GLCcontext* inContext)
{
  __GLCmaster* This = NULL;
  GLCchar32 hashValue =
	((GLCchar32*)GLC_ARRAY_DATA(inContext->masterHashTable))[inMaster];
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcPattern *pattern = NULL;
  int i = 0;

  /* Use Fontconfig to get the default font files */
  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  fontSet = FcFontList(inContext->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  FcPatternDestroy(pattern);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Parse the font set looking for a font with an outline and which hash value
   * matches the hash value of the master we are looking for.
   */
  for (i = 0; i < fontSet->nfont; i++) {
    FcBool outline = FcFalse;
    FcResult result = FcResultMatch;

    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (!outline)
      continue;

    if (hashValue == FcPatternHash(fontSet->fonts[i]))
      break;
  }

  assert(i < fontSet->nfont);

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  /* Duplicate the pattern of the found font (otherwise it will be deleted with
   * the font set).
   */
  This->pattern = FcPatternDuplicate(fontSet->fonts[i]);
  FcFontSetDestroy(fontSet);
  if (!This->pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  return This;
}



/* Destructor of the object */
void __glcMasterDestroy(__GLCmaster* This)
{
  FcPatternDestroy(This->pattern);

  __glcFree(This);
}



/* Get the style name of the face identified by inIndex  */
GLCchar8* __glcMasterGetFaceName(__GLCmaster* This, __GLCcontext* inContext,
				 GLint inIndex)
{
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcResult result = FcResultMatch;
  GLCchar8* string = NULL;
  GLCchar8* faceName;

  objectSet = FcObjectSetBuild(FC_STYLE, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  fontSet = FcFontList(inContext->config, This->pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  if (inIndex >= fontSet->nfont) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    FcFontSetDestroy(fontSet);
    return GLC_NONE;
  }
  result = FcPatternGetString(fontSet->fonts[inIndex], FC_STYLE, 0, &string);
  assert(result != FcResultTypeMismatch);

  faceName = (GLCchar8*)strdup((const char*)string);
  FcFontSetDestroy(fontSet);

  return faceName;
}



/* Is this a fixed font ? */
GLboolean __glcMasterIsFixedPitch(__GLCmaster* This)
{
  FcResult result = FcResultMatch;
  int fixed = 0;

  result = FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);
  assert(result != FcResultTypeMismatch);
  return fixed ? GL_TRUE: GL_FALSE;
}



/* Get the face coutn of the master */
GLint __glcMasterFaceCount(__GLCmaster* This, __GLCcontext* inContext)
{
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  GLint count = 0;

  objectSet = FcObjectSetBuild(FC_STYLE, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }
  fontSet = FcFontList(inContext->config, This->pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  count = fontSet->nfont;
  FcFontSetDestroy(fontSet);

  return count;
}



/* This subroutine is called whenever the user wants to access to informations
 * that have not been loaded from the font files yet. In order to reduce disk
 * accesses, informations such as the master format, full name or version are
 * read "just in time" i.e. only when the user requests them.
 */
GLCchar8* __glcMasterGetInfo(__GLCmaster* This, __GLCcontext* inContext,
			     GLCenum inAttrib)
{
  __GLCfaceDescriptor* faceDesc = NULL;
  FcResult result = FcResultMatch;
  GLCchar8* string = NULL;
  GLCchar *buffer = NULL;

  /* Get the Unicode string which corresponds to the requested attribute */
  switch(inAttrib) {
  case GLC_FAMILY:
    result = FcPatternGetString(This->pattern, FC_FAMILY, 0, &string);
    assert(result != FcResultTypeMismatch);
    break;
  case GLC_VENDOR:
    result = FcPatternGetString(This->pattern, FC_FOUNDRY, 0, &string);
    assert(result != FcResultTypeMismatch);
    break;
  case GLC_VERSION:
  case GLC_FULL_NAME_SGI:
  case GLC_MASTER_FORMAT:
    faceDesc = __glcFaceDescCreate(This, NULL, inContext, 0);

#ifdef GLC_FT_CACHE
    if (!faceDesc) {
#else
    if (!faceDesc || !__glcFaceDescOpen(faceDesc, inContext)) {
#endif
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    string = __glcFaceDescGetFontFormat(faceDesc, inContext, inAttrib);
    break;
  }

  if (string) {
    /* Convert the string and store it in the context buffer */
    buffer = __glcConvertFromUtf8ToBuffer(inContext, string,
					  inContext->stringState.stringType);

    if (!buffer)
      __glcRaiseError(GLC_RESOURCE_ERROR);
  }
  else
    __glcRaiseError(GLC_RESOURCE_ERROR);

  if (faceDesc) {
#ifndef GLC_FT_CACHE
    __glcFaceDescClose(faceDesc);
#endif
    __glcFaceDescDestroy(faceDesc, inContext);
  }

  return buffer;
}



/* Create a master on the basis of the family name */
__GLCmaster* __glcMasterFromFamily(__GLCcontext* inContext, GLCchar8* inFamily)
{
  __GLCmaster* This = NULL;
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcResult result = FcResultMatch;
  int i = 0;

  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (!FcPatternAddString(pattern, FC_FAMILY, inFamily)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }

  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  fontSet = FcFontList(inContext->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  FcPatternDestroy(pattern);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (i = 0; i < fontSet->nfont; i++) {
    FcBool outline = FcFalse;

    /* Check whether the glyphs are outlines */
    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (outline)
      break;
  }

  if (i == fontSet->nfont) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  /* Duplicate the pattern of the found font (otherwise it will be deleted with
   * the font set).
   */
  This->pattern = FcPatternDuplicate(fontSet->fonts[i]);
  FcFontSetDestroy(fontSet);
  if (!This->pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  return This;
}



/* Create a master which contains at least a font which math the character
 * identified by inCode.
 */
__GLCmaster* __glcMasterMatchCode(__GLCcontext* inContext, GLint inCode)
{
  __GLCmaster* This = NULL;
  FcPattern* pattern = NULL;
  FcCharSet* charSet = NULL;
  FcFontSet* fontSet = NULL;
  FcFontSet* fontSet2 = NULL;
  FcObjectSet* objectSet = NULL;
  FcResult result = FcResultMatch;
  int f = 0;

  charSet = FcCharSetCreate();
  if (!charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  if (!FcCharSetAddChar(charSet, inCode)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcCharSetDestroy(charSet);
    return NULL;
  }
  pattern = FcPatternBuild(NULL, FC_CHARSET, FcTypeCharSet, charSet,
			   FC_OUTLINE, FcTypeBool, FcTrue, NULL);
  FcCharSetDestroy(charSet);
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (!FcConfigSubstitute(inContext->config, pattern, FcMatchPattern)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  FcDefaultSubstitute(pattern);
  fontSet = FcFontSort(inContext->config, pattern, FcFalse, NULL, &result);
  FcPatternDestroy(pattern);
  if ((!fontSet) || (result == FcResultTypeMismatch)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (f = 0; f < fontSet->nfont; f++) {
    FcBool outline = FcFalse;

    result = FcPatternGetBool(fontSet->fonts[f], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetCharSet(fontSet->fonts[f], FC_CHARSET, 0, &charSet);
    assert(result != FcResultTypeMismatch);

    if (outline && FcCharSetHasChar(charSet, inCode))
      break;
  }

  if (f == fontSet->nfont) {
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  /* Ugly hack to extract a subset of the pattern fontSet->fonts[f]
   * (otherwise the hash value will not match any value of the hash table).
   */
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }
  fontSet2 = FcFontList(inContext->config, fontSet->fonts[f], objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet2) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  assert(fontSet2->nfont);

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    FcFontSetDestroy(fontSet2);
    return NULL;
  }

  /* Duplicate the pattern of the found font (otherwise it will be deleted with
   * the font set).
   */
  This->pattern = FcPatternDuplicate(fontSet2->fonts[0]);
  FcFontSetDestroy(fontSet2);
  FcFontSetDestroy(fontSet);
  if (!This->pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  return This;
}



GLint __glcMasterGetID(__GLCmaster* This, __GLCcontext* inContext)
{
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(This);
  GLint i = 0;
  GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(inContext->masterHashTable);

  for (i = 0; i < GLC_ARRAY_LENGTH(inContext->masterHashTable); i++) {
    if (hashValue == hashTable[i])
      break;
  }
  assert(i < GLC_ARRAY_LENGTH(inContext->masterHashTable));

  return i;
}
