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

#ifndef __glc_internal_h
#define __glc_internal_h

#include <stdlib.h>

#ifndef DEBUGMODE
#define NDEBUG
#endif
#include <assert.h>

#include "GL/glc.h"
#ifdef HAVE_CONFIG_H
#include "qglc_config.h"
#endif
#include "ofont.h"
#include "ocontext.h"
#include "oarray.h"

#define GLC_OUT_OF_RANGE_LEN	11
#define GLC_EPSILON		1E-6
#define GLC_POINT_SIZE		128

#define GLC_TEXTURE_LOD		1

typedef struct {
  GLint code;
  GLCchar* name;
} __glcDataCodeFromName;

/* Callback function type that is called by __glcProcessChar().
 * It allows to unify the character processing before the rendering or the
 * measurement of a character : __glcProcessChar() is called first (see below)
 * then the callback function of type __glcProcessCharFunc is called by
 * __glcProcessChar(). Two functions are defined according to this type :
 * __glcRenderChar() for rendering and __glcGetCharMetric() for measurement.
 */
typedef void* (*__glcProcessCharFunc)(GLint inCode, GLint inPrevCode,
				      GLint inFont, __glcContextState* inState,
				      void* inProcessCharData,
				      GLboolean inMultipleChars);

/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code or the '\<hexcode>'
 * character sequence is issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__glcContextState *inState, GLint inCode,
		       GLint inPrevCode, __glcProcessCharFunc inProcessCharFunc,
		       void* inProcessCharData);

/* Render scalable characters using either the GLC_LINE style or the
 * GLC_TRIANGLE style
 */
extern void __glcRenderCharScalable(__glcFont* inFont,
				    __glcContextState* inState,
				    GLCenum inRenderMode,
				    GLboolean inDisplayListIsBuilding,
				    GLfloat* inTransformMatrix,
				    GLfloat scale_x, GLfloat scale_y,
				    __glcGlyph* inGlyph);

/* QuesoGLC own memory management routines */
extern void* __glcMalloc(size_t size);
extern void __glcFree(void* ptr);
extern void* __glcRealloc(void* ptr, size_t size);

/* Find a token in a list of tokens separated by 'separator' */
extern GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex,
				   const GLCchar* inSeparator);

/* Arrays that contain the Unicode name of characters */
extern const __glcDataCodeFromName __glcCodeFromNameArray[];
extern const GLint __glcNameFromCodeArray[];
extern const GLint __glcMaxCode;
extern const GLint __glcCodeFromNameSize;

/* Find a Unicode name from its code */
extern GLCchar* __glcNameFromCode(GLint code);

/* Find a Unicode code from its name */
extern GLint __glcCodeFromName(GLCchar* name);

/* Duplicate a string in UTF-8 format */
extern FcChar8* __glcConvertToUtf8(const GLCchar* inString,
				   const GLint inStringType);

/* Duplicate a UTF-8 string to the context buffer */
extern GLCchar* __glcConvertFromUtf8ToBuffer(__glcContextState* This,
					     const FcChar8* inString,
					     const GLint inStringType);

/* Duplicate a counted string in UTF-8 format */
FcChar8* __glcConvertCountedStringToUtf8(const GLint inCount,
					 const GLCchar* inString,
					 const GLint inStringType);

/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. This function is needed since the GLC specs store
 * individual character codes in GLint whatever is their string type.
 */
GLint __glcConvertUcs4ToGLint(__glcContextState *inState, GLint inCode);

/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character
 * codes in GLint whatever is their string type.
 */
GLint __glcConvertGLintToUcs4(__glcContextState *inState, GLint inCode);

/* Verify that the thread has a current context and that the master identified
 * by 'inMaster' exists.
 */
__glcMaster* __glcVerifyMasterParameters(GLint inMaster);

/* Verify that the thread has a current context and that the font identified
 * by 'inFont' exists.
 */
__glcFont* __glcVerifyFontParameters(GLint inFont);

/* Return a struct which contains thread specific info */
threadArea* __glcGetThreadArea(void);

/* Raise an error */
void __glcRaiseError(GLCenum inError);

/* Return the current context state */
__glcContextState* __glcGetCurrent(void);

/* Compute an optimal size for the glyph to be rendered on the screen (if no
 * display list is currently building).
 */
GLboolean __glcGetScale(__glcContextState* inState,
			GLfloat* outTransformMatrix, GLfloat* outScaleX,
			GLfloat* outScaleY);

/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
FcChar32* __glcConvertToVisualUcs4(__glcContextState* inState,
				   const GLCchar* inString);

/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
FcChar32* __glcConvertCountedStringToVisualUcs4(__glcContextState* inState,
						const GLCchar* inString,
						const GLint inCount);

#ifdef FT_CACHE_H
/* Callback function used by the FreeType cache manager to open a given face */
FT_Error __glcFileOpen(FTC_FaceID inFile, FT_Library inLibrary, FT_Pointer inData,
		       FT_Face* outFace);
#endif

/* Save the GL State in a structure */
void __glcSaveGLState(__glcGLState* inGLState);

/* Restore the GL State from a structure */
void __glcRestoreGLState(__glcGLState* inGLState);

#endif /* __glc_internal_h */
