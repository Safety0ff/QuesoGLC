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
 * defines the object __GLCfaceDescriptor that contains the description of a
 * face. One of the purpose of this object is to encapsulate the FT_Face
 * structure from FreeType and to add it some more functionalities.
 * It also allows to centralize the character map management for easier
 * maintenance.
 */

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "internal.h"
#include FT_GLYPH_H
#ifdef FT_CACHE_H
#include FT_CACHE_H
#endif
#include FT_OUTLINE_H

#include FT_TYPE1_TABLES_H
#ifdef FT_XFREE86_H
#include FT_XFREE86_H
#endif
#include FT_BDF_H
#ifdef FT_WINFONTS_H
#include FT_WINFONTS_H
#endif
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the name of the face, the character map, if it is a fixed
 * font or not, the file name and the index of the font in its file.
 */
__GLCfaceDescriptor* __glcFaceDescCreate(__GLCmaster* inMaster,
					 const GLCchar8* inFace,
					 __GLCcontext* inContext)
{
  __GLCfaceDescriptor* This = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcPattern* pattern = inMaster->pattern;
  int i = 0;

  if (inFace) {
    pattern = FcPatternDuplicate(inMaster->pattern);
    if (!pattern) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    if (!FcPatternAddString(pattern, FC_STYLE, inFace)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcPatternDestroy(pattern);
      return NULL;
    }
  }

  objectSet = FcObjectSetBuild(FC_STYLE, FC_SPACING, FC_FILE, FC_INDEX,
			       FC_OUTLINE, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  fontSet = FcFontList(inContext->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (inFace)
    FcPatternDestroy(pattern);

  for (i = 0; i < fontSet->nfont; i++) {
    FcBool outline = FcFalse;
    FcResult result = FcResultMatch;

    /* Check whether the glyphs are outlines */
    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (outline)
      break;
  }

  if (i == fontSet->nfont) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This = (__GLCfaceDescriptor*)__glcMalloc(sizeof(__GLCfaceDescriptor));
  if (!This) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->pattern = FcPatternDuplicate(fontSet->fonts[i]);
  FcFontSetDestroy(fontSet);
  if (!This->pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->node.prev = NULL;
  This->node.next = NULL;
  This->node.data = NULL;
#ifndef FT_CACHE_H
  This->face = NULL;
  This->faceRefCount = 0;
#endif
  This->glyphList.head = NULL;
  This->glyphList.tail = NULL;

  return This;
}



/* Destructor of the object */
void __glcFaceDescDestroy(__GLCfaceDescriptor* This, __GLCcontext* inContext)
{
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

  /* Don't use FT_List_Finalize here, since __glcGlyphDestroy also destroys
   * the node itself.
   */
  node = This->glyphList.head;
  while (node) {
    next = node->next;
    __glcGlyphDestroy((__GLCglyph*)node, inContext);
    node = next;
  }

#if defined(FT_CACHE_H) \
  && (FREETYPE_MAJOR > 2 \
     || (FREETYPE_MAJOR == 2 \
         && (FREETYPE_MINOR > 1 \
             || (FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 8))))
  /* In order to make sure its ID is removed from the FreeType cache */
  FTC_Manager_RemoveFaceID(inContext->cache, (FTC_FaceID)This);
#endif

  FcPatternDestroy(This->pattern);
  __glcFree(This);
}



#ifndef FT_CACHE_H
/* Open a face, select a Unicode charmap. __glcFaceDesc maintains a reference
 * count for each face so that the face is open only once.
 */
FT_Face __glcFaceDescOpen(__GLCfaceDescriptor* This,
			  __GLCcontext* inContext)
{
  if (!This->faceRefCount) {
    GLCchar8 *fileName = NULL;
    int index = 0;
    FcResult result = FcResultMatch;

    /* get the file name */
    result = FcPatternGetString(This->pattern, FC_FILE, 0, &fileName);
    assert(result != FcResultTypeMismatch);
    /* get the index of the font in font file */
    result = FcPatternGetInteger(This->pattern, FC_INDEX, 0, &index);
    assert(result != FcResultTypeMismatch);

    if (FT_New_Face(inContext->library, (const char*)fileName, index,
		    &This->face)) {
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
void __glcFaceDescClose(__GLCfaceDescriptor* This)
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
		       FT_Pointer GLC_UNUSED_ARG(inData), FT_Face* outFace)
{
  __GLCfaceDescriptor* file = (__GLCfaceDescriptor*)inFile;
  GLCchar8 *fileName = NULL;
  int index = 0;
  FcResult result = FcResultMatch;
  FT_Error error;

  /* get the file name */
  result = FcPatternGetString(file->pattern, FC_FILE, 0, &fileName);
  assert(result != FcResultTypeMismatch);
  /* get the index of the font in font file */
  result = FcPatternGetInteger(file->pattern, FC_INDEX, 0, &index);
  assert(result != FcResultTypeMismatch);

  error = FT_New_Face(inLibrary, (const char*)fileName, index, outFace);

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
__GLCglyph* __glcFaceDescGetGlyph(__GLCfaceDescriptor* This, GLint inCode,
				  __GLCcontext* inContext)
{
  FT_Face face = NULL;
  __GLCglyph* glyph = NULL;
  FT_ListNode node = NULL;

  /* Check if the glyph has already been added to the glyph list */
  for (node = This->glyphList.head; node; node = node->next) {
    glyph = (__GLCglyph*)node;
    if (glyph->codepoint == (GLCulong)inCode)
      return glyph;
  }

  /* Open the face */
#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inContext);
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
static FT_Face __glcFaceDescLoadFreeTypeGlyph(__GLCfaceDescriptor* This,
				       __GLCcontext* inContext,
				       GLfloat inScaleX, GLfloat inScaleY,
				       GLCulong inGlyphIndex)
{
  FT_Face face = NULL;
  FT_Int32 loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM;
#ifdef FT_CACHE_H
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
  FTC_FontRec font;
# else
  FTC_ScalerRec scaler;
# endif
  FT_Size size = NULL;
#endif

  /* If GLC_HINTING_QSO is enabled then perform hinting on the glyph while
   * loading it.
   */
  if (!inContext->enableState.hinting)
    loadFlags |= FT_LOAD_NO_HINTING;

  /* Open the face */
#ifdef FT_CACHE_H
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
  font.face_id = (FTC_FaceID)This;
  font.pix_width = (FT_UShort) (inScaleX *
      (inContext->renderState.resolution < GLC_EPSILON ?
       72. : inContext->renderState.resolution) / 72.);
  font.pix_height = (FT_UShort) (inScaleY *
      (inContext->renderState.resolution < GLC_EPSILON ?
       72. : inContext->renderState.resolution) / 72.);

  if (FTC_Manager_Lookup_Size(inContext->cache, &font, &face, &size)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
# else
  scaler.face_id = (FTC_FaceID)This;
  scaler.width = (FT_F26Dot6)(inScaleX * 64.);
  scaler.height = (FT_F26Dot6)(inScaleY * 64.);
  scaler.pixel = 0;
  scaler.x_res = (GLCuint)inContext->renderState.resolution;
  scaler.y_res = (GLCuint)inContext->renderState.resolution;

  if (FTC_Manager_LookupSize(inContext->cache, &scaler, &size)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  face = size->face;
# endif /* FREETYPE_MAJOR */
#else
  face = __glcFaceDescOpen(This, inContext);
  if (!face) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Select the size of the glyph */
  if (FT_Set_Char_Size(face, (FT_F26Dot6)(inScaleX * 64.),
		       (FT_F26Dot6)(inScaleY * 64.),
		       (GLCuint)inContext->resolution,
		       (GLCuint)inContext->resolution)) {
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
void __glcFaceDescDestroyGLObjects(__GLCfaceDescriptor* This,
				   __GLCcontext* inContext)
{
  FT_ListNode node = NULL;

  for (node = This->glyphList.head; node; node = node->next) {
    __GLCglyph* glyph = (__GLCglyph*)node;

    __glcGlyphDestroyGLObjects(glyph, inContext);
  }
}



/* Get the bounding box of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inGlyphIndex' contains the
 * index of the glyph in the font file.
 */
GLfloat* __glcFaceDescGetBoundingBox(__GLCfaceDescriptor* This,
				     GLCulong inGlyphIndex, GLfloat* outVec,
				     GLfloat inScaleX, GLfloat inScaleY,
				     __GLCcontext* inContext)
{
  FT_BBox boundBox;
  FT_Glyph glyph;
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inContext, inScaleX,
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
  outVec[0] = (GLfloat) boundBox.xMin / 64. / inScaleX;
  outVec[2] = (GLfloat) boundBox.xMax / 64. / inScaleX;
  outVec[1] = (GLfloat) boundBox.yMin / 64. / inScaleY;
  outVec[3] = (GLfloat) boundBox.yMax / 64. / inScaleY;

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
#ifdef FT_CACHE_H
GLfloat* __glcFaceDescGetAdvance(__GLCfaceDescriptor* This,
				 GLCulong inGlyphIndex, GLfloat* outVec,
				 GLfloat inScaleX, GLfloat inScaleY,
				 __GLCcontext* inContext)
#else
GLfloat* __glcFaceDescGetAdvance(__GLCfaceDescriptor* This,
				 GLCulong inGlyphIndex, GLfloat* outVec,
				 GLfloat inScaleX, GLfloat inScaleY,
                                 GLboolean inCloseFile,
				 __GLCcontext* inContext)
#endif
{
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inContext, inScaleX,
						inScaleY, inGlyphIndex);

  assert(outVec);

  if (!face)
    return NULL;

  /* Transform the advance according to the conversion from FT_F26Dot6 to
   * GLfloat.
   */
  outVec[0] = (GLfloat) face->glyph->advance.x / 64. / inScaleX;
  outVec[1] = (GLfloat) face->glyph->advance.y / 64. / inScaleY;

#ifndef FT_CACHE_H
  if (inCloseFile)
    __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Use FreeType to determine in which format the face is stored in its file :
 * Type1, TrueType, OpenType, ...
 */
GLCchar8* __glcFaceDescGetFontFormat(__GLCfaceDescriptor* This,
				     __GLCcontext* inContext,
				     GLCenum inAttrib)
{
  static GLCchar8 unknown[] = "Unknown";
#ifndef FT_XFREE86_H
  static GLCchar8 masterFormat1[] = "Type 1";
  static GLCchar8 masterFormat2[] = "BDF";
#  ifdef FT_WINFONTS_H
  static GLCchar8 masterFormat3[] = "Windows FNT";
#  endif /* FT_WINFONTS_H */
  static GLCchar8 masterFormat4[] = "TrueType/OpenType";
#endif /* FT_XFREE86_H */

  FT_Face face = NULL;
  PS_FontInfoRec afont_info;
  const char* acharset_encoding = NULL;
  const char* acharset_registry = NULL;
#ifdef FT_WINFONTS_H
  FT_WinFNT_HeaderRec aheader;
#endif
  GLCuint count = 0;
  GLCchar8* result = NULL;

  /* Open the face */
#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inContext);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

#ifdef FT_XFREE86_H
  if (inAttrib == GLC_MASTER_FORMAT) {
    /* This function is undocumented until FreeType 2.3.0 where it has been
     * added to the public API. It can be safely used nonetheless as long as
     * the existence of FT_XFREE86_H is checked.
     */
    result = (GLCchar8*)FT_Get_X11_Font_Format(face);
#  ifndef FT_CACHE_H
    __glcFaceDescClose(This);
#  endif /* FT_CACHE_H */
    return result;
  }
#endif /* FT_XFREE86_H */

  /* Is it Type 1 ? */
  if (!FT_Get_PS_Font_Info(face, &afont_info)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      return masterFormat1;
#endif
    case GLC_FULL_NAME_SGI:
      if (afont_info.full_name)
	result = (GLCchar8*)afont_info.full_name;
      break;
    case GLC_VERSION:
      if (afont_info.version)
	result = (GLCchar8*)afont_info.version;
      break;
    }
  }
  /* Is it BDF ? */
  else if (!FT_Get_BDF_Charset_ID(face, &acharset_encoding,
				  &acharset_registry)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat2;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }
  }
#ifdef FT_WINFONTS_H
  /* Is it Windows FNT ? */
  else if (!FT_Get_WinFNT_Header(face, &aheader)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat3;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }
  }
#endif
  /* Is it TrueType/OpenType ? */
  else if ((count = FT_Get_Sfnt_Name_Count(face))) {
#if 0
    GLCuint i = 0;
    FT_SfntName aName;
#endif

    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat4;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }

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
  return result;
}



/* Get the maximum metrics of a face that is the bounding box that encloses
 * every glyph of the face, and the maximum advance of the face.
 */
GLfloat* __glcFaceDescGetMaxMetric(__GLCfaceDescriptor* This, GLfloat* outVec,
				   __GLCcontext* inContext)
{
  FT_Face face = NULL;
  /* If the resolution of the context is zero then use the default 72 dpi */
  GLfloat scale = (inContext->renderState.resolution < GLC_EPSILON ?
		   72. : inContext->renderState.resolution) / 72.;

  assert(outVec);

#ifdef FT_CACHE_H
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face))
#else
  face = __glcFaceDescOpen(This, inContext);
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
GLfloat* __glcFaceDescGetKerning(__GLCfaceDescriptor* This,
				 GLCuint inGlyphIndex, GLCuint inPrevGlyphIndex,
				 GLfloat inScaleX, GLfloat inScaleY,
				 GLfloat* outVec, __GLCcontext* inContext)
{
  FT_Vector kerning;
  FT_Error error;
  FT_Face face = __glcFaceDescLoadFreeTypeGlyph(This, inContext, inScaleX,
						inScaleY, inGlyphIndex);

  assert(outVec);

  if (!face)
    return NULL;

  if (!FT_HAS_KERNING(face)) {
    outVec[0] = 0.;
    outVec[1] = 0.;
    return outVec;
  }

  error = FT_Get_Kerning(face, inPrevGlyphIndex, inGlyphIndex,
			 FT_KERNING_DEFAULT, &kerning);

#ifndef FT_CACHE_H
  __glcFaceDescClose(This);
#endif

  if (error)
    return NULL;
  else {
    outVec[0] = (GLfloat) kerning.x / 64. / inScaleX;
    outVec[1] = (GLfloat) kerning.y / 64. / inScaleY;
    return outVec;
  }
}



/* Get the style name of the face descriptor */
GLCchar8* __glcFaceDescGetStyleName(__GLCfaceDescriptor* This)
{
  GLCchar8 *styleName = NULL;
  FcResult result = FcPatternGetString(This->pattern, FC_STYLE, 0, &styleName);

  assert(result != FcResultTypeMismatch);
  return styleName;
}



/* Determine if the face descriptor has a fixed pitch */
GLboolean __glcFaceDescIsFixedPitch(__GLCfaceDescriptor* This)
{
  int fixed = 0;
  FcResult result = FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);

  assert(result != FcResultTypeMismatch);
  return (fixed != FC_PROPORTIONAL);
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * MoveTo is called when the pen move from one curve to another curve.
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcMoveTo(const FT_Vector *inVecTo, void* inUserData)
#else
static int __glcMoveTo(FT_Vector *inVecTo, void* inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;

  /* We don't need to store the point where the pen is since glyphs are defined
   * by closed loops (i.e. the first point and the last point are the same) and
   * the first point will be stored by the next call to lineto/conicto/cubicto.
   */

  if (!__glcArrayAppend(data->endContour,
			&GLC_ARRAY_LENGTH(data->vertexArray))) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;
  return 0;
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * LineTo is called when the pen draws a line between two points.
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcLineTo(const FT_Vector *inVecTo, void* inUserData)
#else
static int __glcLineTo(FT_Vector *inVecTo, void* inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;

  if (!__glcArrayAppend(data->vertexArray, data->vector)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;
  return 0;
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * ConicTo is called when the pen draws a conic between two points (and with
 * one control point).
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcConicTo(const FT_Vector *inVecControl,
			const FT_Vector *inVecTo, void* inUserData)
{
#else
static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo,
			void* inUserData)
{
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  data->vector[2] = (GLfloat)inVecControl->x;
  data->vector[3] = (GLfloat)inVecControl->y;
  data->vector[4] = (GLfloat)inVecTo->x;
  data->vector[5] = (GLfloat)inVecTo->y;
  error = __glcdeCasteljauConic(inUserData);
  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;

  return error;
}



/* Callback functions that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * CubicTo is called when the pen draws a cubic between two points (and with
 * two control points).
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcCubicTo(const FT_Vector *inVecControl1,
			const FT_Vector *inVecControl2,
			const FT_Vector *inVecTo, void* inUserData)
{
#else
static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2,
			FT_Vector *inVecTo, void* inUserData)
{
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  data->vector[2] = (GLfloat)inVecControl1->x;
  data->vector[3] = (GLfloat)inVecControl1->y;
  data->vector[4] = (GLfloat)inVecControl2->x;
  data->vector[5] = (GLfloat)inVecControl2->y;
  data->vector[6] = (GLfloat)inVecTo->x;
  data->vector[7] = (GLfloat)inVecTo->y;
  error = __glcdeCasteljauCubic(inUserData);
  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;

  return error;
}



/* Decompose the outline of a glyph */
GLboolean __glcFaceDescOutlineDecompose(__GLCfaceDescriptor* This,
                                        __GLCrendererData* inData,
                                        __GLCcontext* inContext)
{
  FT_Outline *outline = NULL;
  FT_Outline_Funcs outlineInterface;
  FT_Face face = NULL;

#ifndef FT_CACHE_H
  face = This->face;
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  /* Initialize the data for FreeType to parse the outline */
  outline = &face->glyph->outline;
  outlineInterface.shift = 0;
  outlineInterface.delta = 0;
  outlineInterface.move_to = __glcMoveTo;
  outlineInterface.line_to = __glcLineTo;
  outlineInterface.conic_to = __glcConicTo;
  outlineInterface.cubic_to = __glcCubicTo;

  if (inContext->enableState.glObjects) {
    /* Distances are computed in object space, so is the tolerance of the
     * de Casteljau algorithm.
     */
    inData->tolerance *= face->units_per_EM;
  }

  /* Parse the outline of the glyph */
  if (FT_Outline_Decompose(outline, &outlineInterface, inData)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inData->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inData->endContour) = 0;
    GLC_ARRAY_LENGTH(inData->vertexIndices) = 0;
    GLC_ARRAY_LENGTH(inData->geomBatches) = 0;
    return GL_FALSE;
  }

  return GL_TRUE;
}



/* Compute the lower power of 2 that is greater than value. It is used to
 * determine the smaller texture than can contain a glyph.
 */
static int __glcNextPowerOf2(int value)
{
  int power = 0;

  for (power = 1; power < value; power <<= 1);

  return power;
}



/* Get the size of the bitmap in which the glyph will be rendered */
GLboolean __glcFaceDescGetBitmapSize(__GLCfaceDescriptor* This, GLint* outWidth,
                                     GLint *outHeight, GLint* outBoundingBox,
                                     GLfloat inScaleX, GLfloat inScaleY,
                                     int inFactor, __GLCcontext* inContext)
{
  FT_Face face = NULL;
  FT_Outline outline;
  FT_Matrix matrix;
  FT_BBox boundingBox;


#ifndef FT_CACHE_H
  face = This->face;
  assert(face);
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  outline = face->glyph->outline;

  if (inContext->renderState.renderStyle == GLC_BITMAP) {
    GLfloat *transform = inContext->bitmapMatrix;
    FT_Pos pitch = 0;

    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed)(transform[0] * 65536. / inScaleX);
    matrix.xy = (FT_Fixed)(transform[2] * 65536. / inScaleY);
    matrix.yx = (FT_Fixed)(transform[1] * 65536. / inScaleX);
    matrix.yy = (FT_Fixed)(transform[3] * 65536. / inScaleY);

    /* Get the bounding box of the glyph */
    FT_Outline_Transform(&outline, &matrix);
    FT_Outline_Get_CBox(&outline, &boundingBox);

    /* Compute the sizes of the pixmap where the glyph will be drawn */
    boundingBox.xMin = boundingBox.xMin & -64;	/* floor(xMin) */
    boundingBox.yMin = boundingBox.yMin & -64;	/* floor(yMin) */
    boundingBox.xMax = (boundingBox.xMax + 63) & -64;	/* ceiling(xMax) */
    boundingBox.yMax = (boundingBox.yMax + 63) & -64;	/* ceiling(yMax) */

    /* Calculate pitch to upper 8 byte boundary for 1 bit / pixel, i.e. ceil() */
    pitch = (boundingBox.xMax - boundingBox.xMin + 511) >> 9;
    *outWidth = pitch << 3;
    *outHeight = (boundingBox.yMax - boundingBox.yMin) >> 6;
  }
  else {
    matrix.xy = 0;
    matrix.yx = 0;

    if (inContext->enableState.glObjects) {
      matrix.xx = (FT_Fixed)((GLC_TEXTURE_SIZE << 16) / inScaleX);
      matrix.yy = (FT_Fixed)((GLC_TEXTURE_SIZE << 16) / inScaleY);

      FT_Outline_Transform(&outline, &matrix);
      FT_Outline_Get_CBox(&outline, &boundingBox);

      *outWidth = GLC_TEXTURE_SIZE;
      *outHeight = GLC_TEXTURE_SIZE;

      outline.flags |= FT_OUTLINE_HIGH_PRECISION;
    }
    else {
      matrix.xx = 65536 >> inFactor;
      matrix.yy = 65536 >> inFactor;

      FT_Outline_Transform(&outline, &matrix);
      FT_Outline_Get_CBox(&outline, &boundingBox);

      *outWidth = __glcNextPowerOf2(
              (boundingBox.xMax - boundingBox.xMin + 63) >> 6); /* ceil() */
      *outHeight = __glcNextPowerOf2(
              (boundingBox.yMax - boundingBox.yMin + 63) >> 6); /* ceil() */

      /* If the texture size is too small then give up */
      if ((*outWidth < 4) || (*outHeight < 4))
        return GL_FALSE;
    }
  }

  outBoundingBox[0] = boundingBox.xMin;
  outBoundingBox[1] = boundingBox.yMin;
  outBoundingBox[2] = boundingBox.xMax;
  outBoundingBox[3] = boundingBox.yMax;

  return GL_TRUE;
}



/* Render the glyph in a bitmap */
GLboolean __glcFaceDescGetBitmap(__GLCfaceDescriptor* This, GLint inWidth,
                                 GLint inHeight, void* inBuffer,
                                 __GLCcontext* inContext)
{
  FT_Face face = NULL;
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_Bitmap pixmap;
  FT_Matrix matrix;

#ifndef FT_CACHE_H
  face = This->face;
  assert(face);
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  outline = face->glyph->outline;
  FT_Outline_Get_CBox(&outline, &boundingBox);

  pixmap.width = inWidth;
  pixmap.rows = inHeight;
  pixmap.buffer = inBuffer;

  if (inContext->renderState.renderStyle == GLC_BITMAP) {
    /* Calculate pitch to upper 8 byte boundary for 1 bit / pixel, i.e. ceil() */
    pixmap.pitch = -(pixmap.width >> 3);

    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
  }
  else {
    /* Flip the picture */
    pixmap.pitch = -pixmap.width; /* 8 bits / pixel */

    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_grays; /* Anti-aliased rendering */
    pixmap.num_grays = 256;
  }

  /* fill the pixmap buffer with the background color */
  memset(pixmap.buffer, 0, - pixmap.rows * pixmap.pitch);

  /* translate the outline to match (0,0) with the glyph's lower left
   * corner
   */
  FT_Outline_Translate(&outline, -boundingBox.xMin, -boundingBox.yMin);

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inContext->library, &outline, &pixmap)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  if (inContext->renderState.renderStyle != GLC_BITMAP) {
    /* Prepare the outline for the next mipmap level :
     * a. Restore the outline initial position
     */
    FT_Outline_Translate(&outline, boundingBox.xMin, boundingBox.yMin);

    /* b. Divide the character size by 2. */
    matrix.xx = 32768; /* 0.5 in FT_Fixed type */
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = 32768;
    FT_Outline_Transform(&outline, &matrix);
  }

  return GL_TRUE;
}



/* Chek if the outline of the glyph is empty (which means it is a spacing
 * character).
 */
GLboolean __glcFaceDescOutlineEmpty(__GLCfaceDescriptor* This,
                                    __GLCcontext* inContext)
{
  FT_Face face = NULL;
  FT_Outline outline;

#ifndef FT_CACHE_H
  face = This->face;
  assert(face);
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  outline = face->glyph->outline;

  return outline.n_points ? GL_TRUE : GL_FALSE;
}
