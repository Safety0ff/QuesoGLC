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
 *  defines the so-called "Font commands" described in chapter 3.7 of the GLC
 *  specs.
 */

/** \defgroup font Font commands
 *  Commands to create, manage and destroy fonts.
 *
 *  A font is a stylistically consistent set of glyphs that can be used to
 *  render some set of characters. Each font has a family name (for example
 *  Palatino) and a state variable that selects one of the faces (for
 *  example Regular, Bold, Italic, BoldItalic) that the font contains. A
 *  typeface is the combination of a family and a face (for example
 *  Palatino Bold).
 *
 *  A font is an instantiation of a master for a given face.
 *
 *  Every font has an associated character map. A character map is a table of
 *  entries that maps integer values to the name string that identifies the
 *  characters. The character maps are used by GLC, for instance, to determine
 *  that the character code \e 65 corresponds to \e A. The character map of a
 *  font can be modified by either adding new entries or changing the mapping
 *  of the characters (see glcFontMap()).
 *
 *  GLC maintains two lists of fonts : \b GLC_FONT_LIST and
 *  \b GLC_CURRENT_FONT_LIST. The former contains every font that have been
 *  created with the commands glcNewFontFromFamily() and glcNewFontFromMaster()
 *  and the later contains the fonts that GLC can use when it renders a
 *  character (notice however that if \b GLC_AUTO_FONT is enabled, GLC may
 *  automatically add new fonts from \b GLC_FONT_LIST to the
 *  \b GLC_CURRENT_FONT_LIST ). Finally, it must be stressed that the order in
 *  which the fonts are stored in the \b GLC_CURRENT_FONT_LIST matters : the
 *  first font of the list should be considered as the main font with which
 *  strings are rendered, while other fonts of this list should be seen as
 *  fallback fonts (i.e. fonts that are used when the first font of
 *  \b GLC_CURRENT_FONT_LIST does not map the character to render).
 *
 *  Most of the commands in this category have a parameter \e inFont. Unless
 *  otherwise specified, these commands raise \b GLC_PARAMETER_ERROR if
 *  \e inFont is less than zero or is greater than or equal to the value of
 *  the variable \b GLC_FONT_COUNT.
 */

#include "internal.h"
#include FT_LIST_H



/* Most font commands need to check that :
 *   1. The current thread owns a context state
 *   2. The font identifier 'inFont' is legal
 * This internal function does both checks and returns the pointer to the
 * __GLCfont object that is identified by 'inFont' if the checks have succeeded
 * otherwise returns NULL.
 */
__GLCfont* __glcVerifyFontParameters(GLint inFont)
{
  __GLCcontext *ctx = GLC_GET_CURRENT_CONTEXT();
  FT_ListNode node = NULL;
  __GLCfont *font = NULL;

  /* Check if the current thread owns a context state */
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  /* Verify if the font identifier is in legal bounds */
  for (node = ctx->fontList.head; node; node = node->next) {
    font = (__GLCfont*)node->data;
    if (font->id == inFont) break;
  }

  if (!node) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Returns the __GLCfont object identified by inFont */
  return font;
}



/* Do the actual job of glcAppendFont(). This function can be called as an
 * internal version of glcAppendFont() where the current GLC context is already
 * determined and the font ID has been resolved in its corresponding __GLCfont
 * object.
 */
void __glcAppendFont(__GLCcontext* inContext, __GLCfont* inFont)
{
  FT_ListNode node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));

  if (!node) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

#ifndef FT_CACHE_H
  if (!__glcFaceDescOpen(inFont->faceDesc, inContext)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(node);
    return;
  }
#endif

  /* Add the font to GLC_CURRENT_FONT_LIST */
  node->data = inFont;
  FT_List_Add(&inContext->currentFontList, node);
}



/** \ingroup font
 *  This command appends \e inFont to the list \b GLC_CURRENT_FONT_LIST.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inFont is not an element of
 *  the list \b GLC_FONT_LIST or if \e inFont is an element in the list
 *  \b GLC_CURRENT_FONT_LIST at the beginning of command execution.
 *  \param inFont The ID of the font to append to the list
 *                \b GLC_CURRENT_FONT_LIST
 *  \sa glcGetListc() with argument \b GLC_CURRENT_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_CURRENT_FONT_COUNT
 *  \sa glcFont()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
 */
void APIENTRY glcAppendFont(GLint inFont)
{
  __GLCcontext *ctx = NULL;
  __GLCfont *font = NULL;

  GLC_INIT_THREAD();

  /* Verify that the thread has a current context and that the font identified
   * by 'inFont' exists.
   */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  ctx = GLC_GET_CURRENT_CONTEXT();

  /* Check if inFont is already an element of GLC_CURRENT_FONT_LIST */
  if (FT_List_Find(&ctx->currentFontList, font)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  __glcAppendFont(ctx, font);
}



/* This internal function must be called each time that a command destroys
 * a font. __glcDeleteFont removes, if necessary, the font identified by
 * inFont from the list GLC_CURRENT_FONT_LIST and then delete the font.
 */
static void __glcDeleteFont(__GLCfont* font, __GLCcontext* inContext)
{
  FT_ListNode node = NULL;

  /* Look for the font into GLC_CURRENT_FONT_LIST */
  node = FT_List_Find(&inContext->currentFontList, font);

  /* If the font has been found, remove it from the list */
  if (node) {
    FT_List_Remove(&inContext->currentFontList, node);
#ifndef FT_CACHE_H
    __glcFaceDescClose(font->faceDesc);
#endif
    __glcFree(node);
  }
  __glcFontDestroy(font, inContext);
}



/** \ingroup font
 *  This command deletes the font identified by \e inFont. If \e inFont is an
 *  element in the list \b GLC_CURRENT_FONT_LIST, the command removes that
 *  element from the list.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inFont is not an element of
 *  the list \b GLC_FONT_LIST.
 *  \param inFont The ID of the font to delete
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
 */
void APIENTRY glcDeleteFont(GLint inFont)
{
  __GLCcontext *ctx = NULL;
  __GLCfont *font = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Verify that the thread has a current context and that the font identified
   * by 'inFont' exists.
   */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  ctx = GLC_GET_CURRENT_CONTEXT();

  /* remove the font from the GLC_FONT_LIST then destroy it */
  node = FT_List_Find(&ctx->fontList, font);
  assert(node);
  FT_List_Remove(&ctx->fontList, node);
  __glcFree(node);
  __glcDeleteFont(font, ctx);
}



#ifndef FT_CACHE_H
/* Function called by FT_List_Finalize
 * Close the face of a font when GLC_CURRENT_FONT_LIST is deleted
 */
static void __glcCloseFace(FT_Memory inMemory, void* data, void* user)
{
  __GLCfont* font = (__GLCfont*)data;

  assert(font);
  assert(font->faceDesc);
  __glcFaceDescClose(font->faceDesc);
}
#endif



/** \ingroup font
 *  This command begins by removing all elements from the list
 *  \b GLC_CURRENT_FONT_LIST. If \e inFont is nonzero, the command then appends
 *  \e inFont to the list. Otherwise, the command does not raise an error and
 *  the list remains empty.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inFont is nonzero and is
 *  not an element of the list \b GLC_FONT_LIST.
 *  \param inFont The ID of a font.
 *  \sa glcGetListc() with argument \b GLC_CURRENT_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_CURRENT_FONT_COUNT
 *  \sa glcAppendFont()
 */
void APIENTRY glcFont(GLint inFont)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Verify if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  if (inFont) {
    FT_ListNode node = NULL;
    __GLCfont *font = __glcVerifyFontParameters(inFont);

    /* Verify that the thread has a current context and that the font
     * identified by 'inFont' exists.
     */
    if (!font) {
      /* No need to raise an error since __glcVerifyFontParameters() already
       * did it
       */
      return;
    }

    /* Check if the font is already in GLC_CURRENT_FONT_LIST */
    node = FT_List_Find(&ctx->currentFontList, font);
    if (node) {
      /* Remove the font from the list */
      FT_List_Remove(&ctx->currentFontList, node);
    }
    else {
#ifndef FT_CACHE_H
      if (!__glcFaceDescOpen(font->faceDesc, ctx)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
      }
#endif

      /* Append the font identified by inFont to GLC_CURRENT_FONT_LIST */
      node = ctx->currentFontList.head;
      if (node) {
        /* We keep the first node of the current font list in order not to need
         * to create a new one to store the font identified by 'inFont'
         */
#ifndef FT_CACHE_H
        __GLCfont* dummyFont = (__GLCfont*)node->data;

        /* Close the face of the font stored in the first node */
	__glcFaceDescClose(dummyFont->faceDesc);
#endif
        /* Remove the first node of the list to prevent it to be deleted by
         * FT_List_Finalize().
         */
        FT_List_Remove(&ctx->currentFontList, node);
      }
      else {
        /* The list is empty, create a new node */
        node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
        if (!node) {
#ifndef FT_CACHE_H
	  __glcFaceDescClose(font->faceDesc);
#endif
          __glcRaiseError(GLC_RESOURCE_ERROR);
          return;
        }
      }
    }

#ifndef FT_CACHE_H
    /* Close the remaining fonts in GLC_CURRENT_FONT_LIST and empty the list */
    FT_List_Finalize(&ctx->currentFontList, __glcCloseFace,
		     &__glcCommonArea.memoryManager, NULL);
#else
    /* Empties GLC_CURRENT_FONT_LIST */
    FT_List_Finalize(&ctx->currentFontList, NULL,
		     &__glcCommonArea.memoryManager, NULL);
#endif
    /* Insert the updated node as the first and only node */
    node->data = font;
    FT_List_Add(&ctx->currentFontList, node);
  }
  else {
    /* Empties the list GLC_CURRENT_FONT_LIST */
#ifndef FT_CACHE_H
    FT_List_Finalize(&ctx->currentFontList, __glcCloseFace,
                     &__glcCommonArea.memoryManager, NULL);
#else
    FT_List_Finalize(&ctx->currentFontList, NULL,
                     &__glcCommonArea.memoryManager, NULL);
#endif
  }
}



/* This internal function tries to open the face file which name is identified
 * by 'inFace'. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __GLCfont object "inFont". Otherwise, it leaves the
 * font unchanged. GL_TRUE or GL_FALSE are returned to indicate if the function
 * succeeded or not.
 */
GLboolean __glcFontFace(__GLCfont* inFont, const GLCchar8* inFace,
			__GLCcontext *inContext)
{
  __GLCfaceDescriptor *faceDesc = NULL;
  FcPattern *pattern = NULL;
  FcResult result = FcResultMatch;
  FcCharSet* charSet = NULL;
  __GLCcharMap* newCharMap = NULL;

  GLC_INIT_THREAD();

  /* TODO : Verify if the font has already the required face activated */

  pattern = __glcGetPatternFromMasterID(inFont->parentMasterID, inContext);
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  if (!FcPatternAddString(pattern, FC_STYLE, inFace)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GL_FALSE;
  }

  /* Get the face descriptor of the face identified by the string inFace */
  faceDesc = __glcGetFaceDescFromPattern(pattern, inContext);
  if (!faceDesc) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GL_FALSE;
  }

  result = FcPatternGetCharSet(pattern, FC_CHARSET, 0, &charSet);
  assert(result != FcResultTypeMismatch);

  newCharMap = __glcCharMapCreate(charSet);
  if (!newCharMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return GL_FALSE;
  }

  FcPatternDestroy(pattern);

#ifndef FT_CACHE_H
  /* If the font belongs to GLC_CURRENT_FONT_LIST then open the font file */
  if (FT_List_Find(&inContext->currentFontList, font)) {

    /* Open the new face */
    if (!__glcFaceDescOpen(faceDesc, inContext)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcCharMapDestroy(newCharMap);
      return GL_FALSE;
    }

    /* Close the current face */
    __glcFaceDescClose(font->faceDesc);
  }
#endif

  /* Destroy the current charmap */
  if (inFont->charMap)
    __glcCharMapDestroy(inFont->charMap);

  inFont->charMap = newCharMap;

  __glcFaceDescDestroy(inFont->faceDesc, inContext);
  inFont->faceDesc = faceDesc;

  return GL_TRUE;
}



/** \ingroup font
 *  This command attempts to set the current face of the font identified by
 *  \e inFont to the face identified by the string \e inFace. Examples for
 *  font faces are strings like <tt>"Normal"</tt>, <tt>"Bold"</tt> or
 *  <tt>"Bold Italic"</tt>. In contrast to some systems that have a different
 *  font for each face, GLC allows you to have the face be an attribute of
 *  the font.
 *
 *  If \e inFace is not an element of the font's string list attribute
 *  \b GLC_FACE_LIST, the command leaves the font's current face unchanged and
 *  returns \b GL_FALSE. If the command succeeds, it returns \b GL_TRUE.
 *
 *  If \e inFont is zero, the command iterates over the
 *  \b GLC_CURRENT_FONT_LIST. For each of the fonts named therein, the command
 *  attempts to set the font's current face to the face in that font that is
 *  identified by \e inFace. In this case, the command returns \b GL_TRUE if
 *  \b GLC_CURRENT_FONT_LIST contains one or more elements and the command
 *  successfully sets the current face of each of the fonts named in the list.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inFont is not an element in
 *  the list \b GLC_FONT_LIST.
 *  \param inFont The ID of the font to be changed
 *  \param inFace The face for \e inFont
 *  \return \b GL_TRUE if the command succeeded to set the face \e inFace
 *          to the font \e inFont. \b GL_FALSE is returned otherwise.
 *  \sa glcGetFontFace()
 */
GLboolean APIENTRY glcFontFace(GLint inFont, const GLCchar* inFace)
{
  __GLCcontext *ctx = NULL;
  GLCchar8* UinFace = NULL;
  __GLCfont* font = NULL;

  GLC_INIT_THREAD();

  assert(inFace);

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GL_FALSE;
  }

  UinFace = __glcConvertToUtf8(inFace, ctx->stringState.stringType);
  if (!UinFace) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  if (inFont) {
    GLboolean result = GL_FALSE;

    /* Check if the current thread owns a context state
     * and if the font identified by inFont exists
     */
    font = __glcVerifyFontParameters(inFont);
    if (!font) {
      __glcFree(UinFace);
      return GL_FALSE;
    }

    /* Open the new face */
    result = __glcFontFace(font, UinFace, ctx);
    __glcFree(UinFace);
    return result;
  }
  else {
    FT_ListNode node = NULL;

    if (!ctx->currentFontList.head) {
      __glcFree(UinFace);
      return GL_FALSE;
    }

    /* Check that every font of GLC_CURRENT_FONT_LIST has a face identified
     * by UinFace.
     */
    for (node = ctx->currentFontList.head; node; node = node->next) {
      FcPattern* pattern = NULL;
      __GLCfaceDescriptor* faceDesc = NULL;

      font = (__GLCfont*)node->data;
      pattern = __glcGetPatternFromMasterID(font->parentMasterID, ctx);
      assert(pattern);

      if (!FcPatternAddString(pattern, FC_STYLE, UinFace)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	FcPatternDestroy(pattern);
	return GL_FALSE;
      }

      /* Get the face descriptor of the face identified by the string inFace */
      faceDesc = __glcGetFaceDescFromPattern(pattern, ctx);
      FcPatternDestroy(pattern);
      if (!faceDesc) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return GL_FALSE;
      }

      if (!faceDesc) {
	/* No face identified by UinFace has been found in the font */
	__glcFree(UinFace);
	return GL_FALSE;
      }

      __glcFaceDescDestroy(faceDesc, ctx);
    }

    /* Set the current face of every font of GLC_CURRENT_FONT_LIST to the face
     * identified by UinFace.
     */
    for (node = ctx->currentFontList.head; node; node = node->next)
      __glcFontFace((__GLCfont*)node->data, UinFace, ctx);

    __glcFree(UinFace);
    return GL_TRUE;
  }
}



/** \ingroup font
 *  This command modifies the character map of the font identified by \e inFont
 *  such that the font maps \e inCode to the character whose name is the string
 *  \e inCharName. If \e inCharName is \b GLC_NONE, \e inCode is removed from
 *  the character map.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inCharName is not
 *  \b GLC_NONE or an element of the font string's list attribute
 *  \b GLC_CHAR_LIST.
 *  \param inFont The ID of the font
 *  \param inCode The integer ID of a character
 *  \param inCharName The string name of a character
 *  \sa glcGetFontMap()
 */
void APIENTRY glcFontMap(GLint inFont, GLint inCode, const GLCchar* inCharName)
{
  __GLCfont *font = NULL;
  GLint code = 0;
  __GLCcontext* ctx = NULL;

  GLC_INIT_THREAD();

  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  ctx = GLC_GET_CURRENT_CONTEXT();

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 0)
    return;

  if (!inCharName)
    /* Remove the character from the map */
    __glcCharMapRemoveChar(font->charMap, code);
  else {
    GLCchar* buffer = NULL;
    GLCulong mappedCode  = 0;
    __GLCglyph* glyph = NULL;
    GLint error = 0;

    /* Convert the character name identified by inCharName into UTF-8 format.
     * The result is stored into 'buffer'.
     */
    buffer = __glcConvertToUtf8(inCharName, ctx->stringState.stringType);
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    /* Retrieve the Unicode code from its name */
    error = __glcCodeFromName(buffer);
    if (error < 0) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      __glcFree(buffer);
      return;
    }
    mappedCode = (GLCulong)error;

    /* Get the glyph that corresponds to the mapped code */
    glyph = __glcFaceDescGetGlyph(font->faceDesc, mappedCode, ctx);
    if (!glyph) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      __glcFree(buffer);
      return;
    }
    /* Add the character and its corresponding glyph to the character map */
    __glcCharMapAddChar(font->charMap, inCode, glyph);
    __glcFree(buffer);
  }
}



/** \ingroup font
 *  This command returns a font ID that is not an element of the list
 *  \b GLC_FONT_LIST.
 *  \return A new font ID
 *  \sa glcDeleteFont()
 *  \sa glcIsFont()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
 */
GLint APIENTRY glcGenFontID(void)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Verify if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  /* The context state maintains the lower ID that is greater than all the
   * the IDs of the fonts in GLC_FONT_LIST. So we return this ID and increments
   * 'lastFontID' in order to take into account the new ID returned to the
   * user.
   * Notice however that :
   * - Yes, this is a lazy policy
   * - Even if the user never creates a font with this ID, QuesoGLC will
   *   assume that this ID is lost until the context is deleted. This may lead
   *   to unused font IDs.
   * - If the user calls glcGenFontID() twice in a row he will get 2 different
   *   IDs even though he/she has not used the first one. This may not be the
   *   expected behaviour.
   * - We do not take into account the overfow of the lastFontID parameter
   *   and an evil user can make QuesoGLC returns the ID of a font that is
   *   already a member of GLC_FONT_LIST
   * But that would be the point in checking for such events ? Even if
   * glcGenFontID() returns silly IDs, *every* GLC function that requires a
   * font ID as a parameter checks its validity before using it. So yes, this
   * function is really stupid but since it does not harm, let's keep it this
   * way. In most cases, it will behave in a sensible way.
   * Moreover, this behaviour is compliant with GLC specs and as a last resort
   * the client can manage the font IDs itself.
   */
  return ctx->lastFontID++;
}



/** \ingroup font
 *  This command returns the string name of the current face of the font
 *  identified by \e inFont.
 *  \param inFont The font ID
 *  \return The string name of the font \e inFont
 *  \sa glcFontFace()
 */
const GLCchar* APIENTRY glcGetFontFace(GLint inFont)
{
  __GLCfont *font = NULL;

  GLC_INIT_THREAD();

  font = __glcVerifyFontParameters(inFont);
  if (font) {
    GLCchar *buffer = NULL;
    __GLCcontext *ctx = GLC_GET_CURRENT_CONTEXT();

    /* Convert the string name of the face into the current string type */
    buffer = __glcConvertFromUtf8ToBuffer(ctx, font->faceDesc->styleName,
					  ctx->stringState.stringType);
    if (!buffer) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return GLC_NONE;
    }

    /* returns the name */
    return buffer;
  }
  else
    return GLC_NONE;
}



/** \ingroup font
 *  This command returns an attribute of the font identified by \e inFont that
 *  is a string from a string list identified by \e inAttrib. The command
 *  returns the string at offset \e inIndex from the first element in \e
 *  inAttrib. For example, if \e inFont has a face list (\c Regular, \c Bold,
 *  \c Italic ) and \e inIndex is \c 2, then the command returns \c Italic if
 *  you query \b GLC_FACE_LIST.
 *
 *  Every GLC state variable that is a list has an associated integer element
 *  count whose value is the number of elements in the list.
 *
 *  Below are the string list attributes associated with each GLC master and
 *  font and their element count attributes :
 * <center>
 * <table>
 * <caption>Master/font string list attributes</caption>
 *   <tr>
 *     <td>Name</td> <td>Enumerant</td> <td>Element count attribute</td>
 *   </tr>
 *   <tr>
 *     <td><b>GLC_CHAR_LIST</b></td>
 *     <td>0x0050</td>
 *     <td><b>GLC_CHAR_COUNT</b></td>
 *   </tr>
 *   <tr>
 *     <td><b>GLC_FACE_LIST</b></td>
 *     <td>0x0051</td>
 *     <td><b>GLC_FACE_COUNT</b></td>
 *   </tr>
 * </table>
 * </center>
 *  \n The command raises \b GLC_PARAMETER_ERROR if \e inIndex is less than
 *  zero or is greater than or equal to the value of the list element count
 *  attribute.
 *  \param inFont The font ID
 *  \param inAttrib The string list from which a string is requested
 *  \param inIndex The offset from the first element of the list associated
 *                 with \e inAttrib
 *  \return A string attribute of \e inFont identified by \e inAttrib
 *  \sa glcGetMasterListc()
 *  \sa glcGetFontc()
 *  \sa glcGetFonti()
 */
const GLCchar* APIENTRY glcGetFontListc(GLint inFont, GLCenum inAttrib,
					GLint inIndex)
{
  __GLCfont* font = NULL;
  __GLCcontext* ctx = NULL;

  GLC_INIT_THREAD();

  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return;

  ctx = GLC_GET_CURRENT_CONTEXT();

  switch(inAttrib) {
  case GLC_FACE_LIST:
    return glcGetMasterListc(font->parentMasterID, inAttrib, inIndex);
  case GLC_CHAR_LIST:
    return __glcCharMapGetCharNameByIndex(font->charMap, inIndex, ctx);
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
}



/** \ingroup font
 *  This command returns the string name of the character that the font
 *  identified by \e inFont maps \e inCode to.
 *
 *  To change the map, that is, associate a different name string with the
 *  integer ID of a font, use glcFontMap().
 *
 *  If \e inCode cannot be mapped in the font, the command returns \b GLC_NONE.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inFont is not an element of
 *  the list \b GLC_FONT_LIST
 *  \note Changing the map of a font is possible but changing the map of a
 *        master is not.
 *  \param inFont The integer ID of the font from which to select the character
 *  \param inCode The integer ID of the character in the font map
 *  \return The string name of the character that the font \e inFont maps
 *          \e inCode to.
 *  \sa glcGetMasterMap()
 *  \sa glcFontMap()
 */
const GLCchar* APIENTRY glcGetFontMap(GLint inFont, GLint inCode)
{
  __GLCfont *font = NULL;
  __GLCcontext *ctx = NULL;
  GLint code = 0;

  GLC_INIT_THREAD();

  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return GLC_NONE;

  ctx = GLC_GET_CURRENT_CONTEXT();

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 0)
    return GLC_NONE;

  return __glcCharMapGetCharName(font->charMap, code, ctx);
}



/** \ingroup font
 *  This command returns a string attribute of the font identified by
 *  \e inFont. The table below lists the string attributes that are
 *  associated with each GLC master and font.
 *  <center>
 *  <table>
 *  <caption>Master/font string attributes</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FAMILY</b></td> <td>0x0060</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MASTER_FORMAT</b></td> <td>0x0061</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_VENDOR</b></td> <td>0x0062</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_VERSION</b></td> <td>0x0063</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FULL_NAME_SGI</b></td> <td>0x8002</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inFont The font for which the attribute is requested
 *  \param inAttrib The requested string attribute
 *  \return The string attribute \e inAttrib of the font \e inFont
 *  \sa glcGetMasterc()
 *  \sa glcGetFonti()
 *  \sa glcGetFontListc()
 */
const GLCchar* APIENTRY glcGetFontc(GLint inFont, GLCenum inAttrib)
{
  __GLCfont *font = NULL;

  GLC_INIT_THREAD();

  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (font)
    /* Those are master attributes so we call the equivalent master function */
    return glcGetMasterc(font->parentMasterID, inAttrib);
  else
    return GLC_NONE;
}



/** \ingroup font
 *  This command returns an integer attribute of the font identified by
 *  \e inFont. The attribute is identified by \e inAttrib. The table below
 *  lists the integer attributes that are associated with each GLC master
 *  and font.
 *  <center>
 *  <table>
 *  <caption>Master/font integer attributes</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_CHAR_COUNT</b></td> <td>0x0070</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_FACE_COUNT</b></td> <td>0x0071</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_IS_FIXED_PITCH</b></td> <td>0x0072</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MAX_MAPPED_CODE</b></td> <td>0x0073</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_MIN_MAPPED_CODE</b></td> <td>0x0074</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inFont The font for which the attribute is requested.
 *  \param inAttrib The requested integer attribute
 *  \return The value of the specified integer attribute
 *  \sa glcGetMasteri()
 *  \sa glcGetFontc()
 *  \sa glcGetFontListc()
 */
GLint APIENTRY glcGetFonti(GLint inFont, GLCenum inAttrib)
{
  __GLCfont *font = NULL;

  GLC_INIT_THREAD();

  /* Check if the font parameters are valid */
  font = __glcVerifyFontParameters(inFont);
  if (!font)
    return 0;

  switch(inAttrib) {
  case GLC_CHAR_COUNT:
    return __glcCharMapGetCount(font->charMap);
  case GLC_IS_FIXED_PITCH:
    return font->faceDesc->isFixedPitch;
  case GLC_MAX_MAPPED_CODE:
    return __glcCharMapGetMaxMappedCode(font->charMap);
  case GLC_MIN_MAPPED_CODE:
    return __glcCharMapGetMinMappedCode(font->charMap);
  case GLC_FACE_COUNT:
    return glcGetMasteri(font->parentMasterID, inAttrib);
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }
}



/** \ingroup font
 *  This command returns \b GL_TRUE if \e inFont is the ID of a font. If
 *  \e inFont is not the ID of a font, the command does not raise an error.
 *  \param inFont The element to be tested
 *  \return \b GL_TRUE if \e inFont is the ID of a font, \b GL_FALSE
 *          otherwise.
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcNewFontFromMaster()
 */
GLboolean APIENTRY glcIsFont(GLint inFont)
{
  __GLCcontext *ctx = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return GL_FALSE;
  }

  /* Verify if the font identifier exists */
  for (node = ctx->fontList.head; node; node = node->next) {
    __GLCfont *font = (__GLCfont*)node->data;
    if (font->id == inFont)
      return GL_TRUE;
  }

  return GL_FALSE;
}



/* This internal function deletes the font identified by inFont (if any) and
 * creates a new font based on the pattern 'inPattern'. The resulting font is
 * added to the list GLC_FONT_LIST.
 */
__GLCfont* __glcNewFontFromMaster(__GLCfont* inFont, GLint inFontID,
				  FcPattern* inPattern, __GLCcontext *inContext)
{
  FT_ListNode node = NULL;
  __GLCfont* font = NULL;

  GLC_INIT_THREAD();

  /* Create a new entry for GLC_FONT_LIST */
  node = (FT_ListNode)__glcMalloc(sizeof(FT_ListNodeRec));
  if (!node) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (inFont)
    /* Delete the font */
    __glcDeleteFont(inFont, inContext);

  /* Create a new font and add it to the list GLC_FONT_LIST */
  font = __glcFontCreate(inFontID, inPattern, inContext);
  if (!font) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(node);
    return NULL;
  }
  node->data = font;
  FT_List_Add(&inContext->fontList, node);

  /* Update the last font ID (see glcGenFontID())*/
  if (inFontID >= inContext->lastFontID)
    inContext->lastFontID = inFontID + 1;
  return font;
}



/** \ingroup font
 *  This command creates a new font from the master identified by \e inMaster.
 *  The ID of the new font is \e inFont. If the command succeeds, it returns
 *  \e inFont. If \e inFont is the ID of a font at the beginning of command
 *  execution, the command executes the command \c glcDeleteFont(inFont) before
 *  creating the new font.
 *  \param inFont The ID of the new font
 *  \param inMaster The master from which to create the new font
 *  \return The ID of the new font if the command succceeds, \c 0 otherwise.
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromFamily()
 *  \sa glcDeleteFont()
 */
GLint APIENTRY glcNewFontFromMaster(GLint inFont, GLint inMaster)
{
  __GLCcontext *ctx = NULL;
  FcPattern *pattern = NULL;
  __GLCfont *font = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Verify that the thread has a current context and that the master
   * identified by 'inMaster' exists.
   */
  pattern = __glcVerifyMasterParameters(inMaster);
  if (!pattern)
    return 0;

  ctx = GLC_GET_CURRENT_CONTEXT();

  /* Check if inFont is in legal bounds */
  if (inFont < 1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Look for the font which ID is inFont in the GLC_FONT_LIST */
  for (node = ctx->fontList.head; node; node = node->next) {
    font = (__GLCfont*)node->data;
    if (font->id == inFont)
      break;
  }

  /* No font with an ID equal to inFont has been found */
  if (!node)
    font = NULL;

  /* Create and return the new font */
  font = __glcNewFontFromMaster(font, inFont, pattern, ctx);
  FcPatternDestroy(pattern);
  return font->id;
}



/** \ingroup font
 *  This command performs a sequential search beginning with the first element
 *  of the GLC master list, looking for the first master whose string
 *  attribute \b GLC_FAMILY equals \e inFamily. If there is no such master the
 *  command returns zero. Otherwise, the command creates a new font from the
 *  master. The ID of the new font is \e inFont.
 *
 *  If the command succeeds, it returns \e inFont. If \e inFont is the ID of a
 *  font at the beginning of command execution, the command executes the
 *  command \c glcDeleteFont(inFont) before creating the new font.
 *  \param inFont The ID of the new font.
 *  \param inFamily The font family, that is, the string that \b GLC_FAMILY
 *                  attribute has to match.
 *  \return The ID of the new font if the command succeeds, \c 0 otherwise.
 *  \sa glcGetListi() with argument \b GLC_FONT_LIST
 *  \sa glcGeti() with argument \b GLC_FONT_COUNT
 *  \sa glcIsFont()
 *  \sa glcGenFontID()
 *  \sa glcNewFontFromMaster()
 *  \sa glcDeleteFont()
 */
GLint APIENTRY glcNewFontFromFamily(GLint inFont, const GLCchar* inFamily)
{
  __GLCcontext *ctx = NULL;
  GLCchar8* UinFamily = NULL;
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcResult result = FcResultMatch;
  int i = 0;
  __GLCfont *font = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  assert(inFamily);

  /* Check if inFont is in legal bounds */
  if (inFont < 1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return 0;
  }

  /* Verify if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return 0;
  }

  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  /* Convert the family name in UTF-8 encoding */
  UinFamily = __glcConvertToUtf8(inFamily, ctx->stringState.stringType);
  if (!UinFamily) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return 0;
  }

  if (!FcPatternAddString(pattern, FC_FAMILY, UinFamily)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(UinFamily);
    FcPatternDestroy(pattern);
    return 0;
  }

  __glcFree(UinFamily);

  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       FC_CHARSET, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return 0;
  }
  fontSet = FcFontList(ctx->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  FcPatternDestroy(pattern);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
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
    return 0;
  }

  /* Look for the font which ID is inFont in the GLC_FONT_LIST */
  for (node = ctx->fontList.head; node; node = node->next) {
    font = (__GLCfont*)node->data;
    if (font->id == inFont)
      break;
  }

  /* No font with an ID equal to inFont has been found */
  if (!node)
    font = NULL;

  /* Create and return the new font */
  font = __glcNewFontFromMaster(font, inFont, fontSet->fonts[i], ctx);
  FcFontSetDestroy(fontSet);
  return font->id;
}
