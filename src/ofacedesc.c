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

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "internal.h"
#include FT_LIST_H
#include FT_GLYPH_H
#ifdef FT_CACHE_H
#include FT_CACHE_H
#endif

#include FT_TYPE1_TABLES_H
#include FT_BDF_H
#ifdef FT_WINFONTS_H
#include FT_WINFONTS_H
#endif
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

/** \file
 * defines the object __glcFaceDesc that contains the description of a face.
 * One of the purpose of this object is to encapsulate the FT_Face structure
 * from FreeType and to add it some more functionalities.
 * It also allows to centralize the character map management for easier
 * maintenance.
 */

/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the name of the face, the character map, if it is a fixed
 * font or not, the file name and the index of the font in its file.
 */
__glcFaceDescriptor* __glcFaceDescCreate(FcChar8* inStyleName,
					 FcCharSet* inCharSet,
					 GLboolean inIsFixedPitch,
					 FcChar8* inFileName,
					 FT_Long inIndexInFile)
{
  __glcFaceDescriptor* This = NULL;

  This = (__glcFaceDescriptor*)__glcMalloc(sizeof(__glcFaceDescriptor));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->styleName = (FcChar8*)strdup((const char*)inStyleName);
  if (!This->styleName) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  /* Filenames are kept in their original format which is supposed to
   * be compatible with strlen()
   */
  This->fileName = (FcChar8*)strdup((const char*)inFileName);
  if (!This->fileName) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    free(This->styleName);
    __glcFree(This);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;
  This->isFixedPitch = inIsFixedPitch;
  This->indexInFile = inIndexInFile;
#ifndef FT_CACHE_H
  This->face = NULL;
  This->faceRefCount = 0;
#endif
  This->glyphList.head = NULL;
  This->glyphList.tail = NULL;

  return This;
}



/* Destructor of the object */
void __glcFaceDescDestroy(__glcFaceDescriptor* This, __glcContextState* inState)
{
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

  /* Don't use FT_List_Finalize here, since __glcGlyphDestroy also destroys
   * the node itself.
   */
  node = This->glyphList.head;
  while (node) {
    next = node->next;
    __glcGlyphDestroy((__glcGlyph*)node);
    node = next;
  }

#ifdef FT_CACHE_H
  /* In order to make sure its ID is removed from the FreeType cache */
  FTC_Manager_RemoveFaceID(inState->cache, (FTC_FaceID)This);
#endif

  free(This->styleName);
  free(This->fileName);
  __glcFree(This);
}



#ifndef FT_CACHE_H
/* Open a face, select a Unicode charmap. __glcFaceDesc maintains a reference
 * count for each face so that the face is open only once.
 */
FT_Face __glcFaceDescOpen(__glcFaceDescriptor* This,
			  __glcContextState* inState)
{
  if (!This->faceRefCount) {
    if (FT_New_Face(inState->library, (const char*)This->fileName,
		    This->indexInFile, &This->face)) {
      /* Unable to load the face file */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    /* select a Unicode charmap */
    if (FT_Select_Charmap(This->face, ft_encoding_unicode)) {
      /* No Unicode charmap is available */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FT_Done_Face(This->face);
      This->face = NULL;
      return NULL;
    }

    This->faceRefCount = 1;
  }
  else
    This->faceRefCount++;

  return This->face;
}



/* Close the face and update the reference counter accordingly */
void __glcFaceDescClose(__glcFaceDescriptor* This)
{
  assert(This->faceRefCount > 0);

  This->faceRefCount--;

  if (!This->faceRefCount) {
    assert(This->face);

    FT_Done_Face(This->face);
    This->face = NULL;
  }
}



#else /* FT_CACHE_H */
/* Callback function used by the FreeType cache manager to open a given face */
FT_Error __glcFileOpen(FTC_FaceID inFile, FT_Library inLibrary,
		       FT_Pointer inData, FT_Face* outFace)
{
  __glcFaceDescriptor* file = (__glcFaceDescriptor*)inFile;
  FT_Error error = FT_New_Face(inLibrary, (const char*)file->fileName,
			       file->indexInFile, outFace);

  if (error) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return error;
  }

  /* select a Unicode charmap */
  error = FT_Select_Charmap(*outFace, ft_encoding_unicode);
  if (error)
    __glcRaiseError(GLC_RESOURCE_ERROR);

  return error;
}
#endif /* FT_CACHE_H */



/* Return the glyph which corresponds to codepoint 'inCode' */
__glcGlyph* __glcFaceDescGetGlyph(__glcFaceDescriptor* This, GLint inCode,
				  __glcContextState* inState)
{
  FT_Face face = NULL;
  __glcGlyph* glyph = NULL;
  FT_ListNode node = NULL;

  /* Check if the glyph has already been added to the glyph list */
  for (node = This->glyphList.head; node; node = node->next) {
    glyph = (__glcGlyph*)node;
    if (glyph->codepoint == inCode)
      return glyph;
  }

  /* Open the face */
#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inState->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inState);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Create a new glyph */
#ifdef FT_CACHE_H
  glyph = __glcGlyphCreate(FT_Get_Char_Index(face, inCode), inCode);
  if (!glyph)
    return NULL;
#else
  glyph = __glcGlyphCreate(FcFreeTypeCharIndex(face, inCode), inCode);
  if (!glyph) {
    __glcFaceDescClose(This);
    return NULL;
  }
#endif
  /* Append the new glyph to the list of the glyphes of the face and close the
   * face.
   */
  FT_List_Add(&This->glyphList, (FT_ListNode)glyph);
#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif
  return glyph;
}



/* Load a FreeType glyph (FT_Glyph) of the current face and returns the 
 * corresponding FT_Face. The size of the glyph is given by inScaleX and
 * inScaleY. 'inGlyphIndex' contains the index of the glyph in the font file.
 */
FT_Face __glcFaceDescLoadFreeTypeGlyph(__glcFaceDescriptor* This,
				       __glcContextState* inState,
				       GLfloat inScaleX, GLfloat inScaleY,
				       FT_ULong inGlyphIndex)
{
  FT_Face face = NULL;
  FT_Int32 loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM;
#ifdef FT_CACHE_H
  FTC_ScalerRec scaler;
  FT_Size size = NULL;
#endif

  /* If GLC_HINTING_QSO is enabled then perform hinting on the glyph while
   * loading it.
   */
  if (!inState->enableState.hinting)
    loadFlags |= FT_LOAD_NO_HINTING;

  /* Open the face */
#ifdef FT_CACHE_H
  scaler.face_id = (FTC_FaceID)This;
  scaler.width = (FT_F26Dot6)(inScaleX * 64.);
  scaler.height = (FT_F26Dot6)(inScaleY * 64.);
  scaler.pixel = 0;
  scaler.x_res = (FT_UInt)inState->renderState.resolution;
  scaler.y_res = (FT_UInt)inState->renderState.resolution;

  if (FTC_Manager_LookupSize(inState->cache, &scaler, &size)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  face = size->face;
#else
  face = __glcFaceDescOpen(This, inState);
  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Select the size of the glyph */
  if (FT_Set_Char_Size(face, (FT_F26Dot6)(inScaleX * 64.),
		       (FT_F26Dot6)(inScaleY * 64.),
		       (FT_UInt)inState->resolution,
		       (FT_UInt)inState->resolution)) {
    __glcFaceDescClose(This);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
#endif

  /* Load the glyph */
  if (FT_Load_Glyph(face, inGlyphIndex, loadFlags)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifndef FT_CACHE_H
    __glcFaceDescClose(This);
#endif
    return NULL;
  }

  return face;
}



/* Destroy the GL objects of every glyph of the face */
void __glcFaceDescDestroyGLObjects(__glcFaceDescriptor* This)
{
  FT_ListNode node = NULL;

  for (node = This->glyphList.head; node; node = node->next) {
    __glcGlyph* glyph = (__glcGlyph*)node;

    __glcGlyphDestroyGLObjects(glyph);
  }
}



/* Get a copy of the character map of the face */
__glcCharMap* __glcFaceDescGetCharMap(__glcFaceDescriptor* This,
				      __glcContextState* inState)
{
  __glcCharMap* charMap = NULL;
  FT_Face face = NULL;
  FcCharSet* charSet = NULL;
#ifdef FT_CACHE_H
  FT_UInt gindex = 0;
  FT_ULong charCode = 0;
#endif

  /* Check that the face is opened */
#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inState->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inState);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Build a character map based on the FcCharSet of the face */
#ifndef FT_CACHE_H
  charSet = FcFreeTypeCharSet(face, NULL);
#else
  /* For some reason, FcFreeTypeCharSet() fails when a cached face is used.
   * Instead we build the charset ourselves but that means that we may not
   * get a list as complete as the one we get when calling FcFreeTypeCharSet.
   */
  charSet = FcCharSetCreate();
  if (!charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  charCode = FT_Get_First_Char(face, &gindex);
  while (gindex) {
    if (!FcCharSetAddChar(charSet, charCode)) {
      FcCharSetDestroy(charSet);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    charCode = FT_Get_Next_Char(face, charCode, &gindex);
  }
#endif

  charMap = __glcCharMapCreate(charSet);
  FcCharSetDestroy(charSet);
#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif
  return charMap;
}



/* Get the bounding box of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inGlyphIndex' contains the
 * index of the glyph in the font file.
 */
GLfloat* __glcFaceDescGetBoundingBox(__glcFaceDescriptor* This,
				     FT_ULong inGlyphIndex, GLfloat* outVec,
				     GLfloat inScaleX, GLfloat inScaleY,
				     __glcContextState* inState)
{
  FT_BBox boundBox;
  FT_Glyph glyph;
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inState, inScaleX,
						inScaleY, inGlyphIndex);

  assert(outVec);

  if (!face)
    return NULL;

  /* Get the bounding box of the glyph */
  FT_Get_Glyph(face->glyph, &glyph);
  FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &boundBox);

  /* Transform the bounding box according to the conversion from FT_F26Dot6 to
   * GLfloat and the size in points of the glyph.
   */
  outVec[0] = (GLfloat) boundBox.xMin / GLC_POINT_SIZE / 64.;
  outVec[2] = (GLfloat) boundBox.xMax / GLC_POINT_SIZE / 64.;
  outVec[1] = (GLfloat) boundBox.yMin / GLC_POINT_SIZE / 64.;
  outVec[3] = (GLfloat) boundBox.yMax / GLC_POINT_SIZE / 64.;

  FT_Done_Glyph(glyph);
#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Get the advance of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inGlyphIndex' contains the
 * index of the glyph in the font file.
 */
GLfloat* __glcFaceDescGetAdvance(__glcFaceDescriptor* This,
				 FT_ULong inGlyphIndex, GLfloat* outVec,
				 GLfloat inScaleX, GLfloat inScaleY,
				 __glcContextState* inState)
{
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inState, inScaleX,
						inScaleY, inGlyphIndex);

  assert(outVec);

  if (!face)
    return NULL;

  /* Transform the advance according to the conversion from FT_F26Dot6 to
   * GLfloat.
   */
  outVec[0] = (GLfloat) face->glyph->advance.x / 64.;
  outVec[1] = (GLfloat) face->glyph->advance.y / 64.;

#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Use FreeType to determine in which format the face is stored in its file :
 * Type1, TrueType, OpenType, ...
 */
GLboolean __glcFaceDescGetFontFormat(__glcFaceDescriptor* This,
				     __glcContextState* inState,
				     FcChar8** inFormat,
				     FcChar8** inFullNameSGI,
				     FcChar8** inVersion)
{
  static FcChar8 unknown[] = "Unknown";
  static FcChar8 masterFormat1[] = "Type 1";
  static FcChar8 masterFormat2[] = "BDF";
#ifdef FT_WINFONTS_H
  static FcChar8 masterFormat3[] = "Windows FNT";
#endif
  static FcChar8 masterFormat4[] = "TrueType/OpenType";

  FT_Face face = NULL;
  PS_FontInfoRec afont_info;
  const char* acharset_encoding = NULL;
  const char* acharset_registry = NULL;
#ifdef FT_WINFONTS_H
  FT_WinFNT_HeaderRec aheader;
#endif
  FT_UInt count = 0;

  /* Open the face */
#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inState->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inState);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  /* Is it Type 1 ? */
  if (!FT_Get_PS_Font_Info(face, &afont_info)) {
    *inFormat = masterFormat1;
    if (afont_info.full_name)
      *inFullNameSGI = (FcChar8*)strdup(afont_info.full_name);
    if (afont_info.version)
      *inVersion = (FcChar8*)strdup(afont_info.version);
  }
  /* Is it BDF ? */
  else if (!FT_Get_BDF_Charset_ID(face, &acharset_encoding,
				  &acharset_registry)) {
    *inFormat = masterFormat2;
    *inFullNameSGI = unknown;
    *inVersion = unknown;
  }
#ifdef FT_WINFONTS_H
  /* Is it Windows FNT ? */
  else if (!FT_Get_WinFNT_Header(face, &aheader)) {
    *inFormat = masterFormat3;
    *inFullNameSGI = unknown;
    *inVersion = unknown;
  }
#endif
  /* Is it TrueType/OpenType ? */
  else if ((count = FT_Get_Sfnt_Name_Count(face))) {
#if 0
    FT_UInt i = 0;
    FT_SfntName aName;
#endif

    *inFormat = masterFormat4;
    *inFullNameSGI = unknown;
    *inVersion = unknown;

    /* TODO : decode the SFNT name tables in order to get full name
     * of the TrueType/OpenType fonts and their version
     */
#if 0
    for (i = 0; i < count; i++) {
      if (!FT_Get_Sfnt_Name(face, i, &aName)) {
        if ((aName.name_id != TT_NAME_ID_FULL_NAME)
		&& (aName.name_id != TT_NAME_ID_VERSION_STRING))
	  continue;

        switch (aName.platform_id) {
	case TT_PLATFORM_APPLE_UNICODE:
	  break;
	case TT_PLATFORM_MICROSOFT:
	  break;
	}
      }
    }
#endif
  }

#ifndef FT_CACHE_H
  /* Close the face */
  __glcFaceDescClose(This);
#endif
  return GL_TRUE;
}



/* Get the maximum metrics of a face that is the bounding box that encloses
 * every glyph of the face, and the maximum advance of the face.
 */
GLfloat* __glcFaceDescGetMaxMetric(__glcFaceDescriptor* This, GLfloat* outVec,
				   __glcContextState* inState)
{
  FT_Face face = NULL;
  /* If the resolution of the context is zero then use the default 72 dpi */
  GLfloat scale = (inState->renderState.resolution < GLC_EPSILON ?
		   72. : inState->renderState.resolution) / 72.;

  assert(outVec);

#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inState->cache, (FTC_FaceID)This, &face))
#else
  face = __glcFaceDescOpen(This, inState);
  if (!face)
#endif
    return NULL;

  scale /= face->units_per_EM;

  /* Get the values and transform them according to the resolution */
  outVec[0] = (GLfloat)face->max_advance_width * scale;
  outVec[1] = (GLfloat)face->max_advance_height * scale;
  outVec[2] = (GLfloat)face->bbox.yMax * scale;
  outVec[3] = (GLfloat)face->bbox.yMin * scale;
  outVec[4] = (GLfloat)face->bbox.xMax * scale;
  outVec[5] = (GLfloat)face->bbox.xMin * scale;

#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Get the kerning information of a pair of glyphes according to the size given
 * by inScaleX and inScaleY. The result is returned in outVec.
 */
GLfloat* __glcFaceDescGetKerning(__glcFaceDescriptor* This,
				 FT_UInt inGlyphIndex, FT_UInt inPrevGlyphIndex,
				 GLfloat inScaleX, GLfloat inScaleY,
				 GLfloat* outVec, __glcContextState* inState)
{
  FT_Vector kerning;
  FT_Error error;
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inState, inScaleX,
						inScaleY, inGlyphIndex);

  assert(outVec);

  if (!face)
    return NULL;

  error = FT_Get_Kerning(face, inPrevGlyphIndex, inGlyphIndex,
			 FT_KERNING_DEFAULT, &kerning);

#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif

  if (error)
    return NULL;
  else {
    outVec[0] = (GLfloat) kerning.x / 64.;
    outVec[1] = (GLfloat) kerning.y / 64.;
    return outVec;
  }
}
