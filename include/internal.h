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
#if !defined __APPLE__ && !defined __MACH__
#include "qglc_config.h"
#endif
#include "ofont.h"
#include "ocontext.h"
#include "oarray.h"

#define GLC_OUT_OF_RANGE_LEN	11
#define GLC_EPSILON		1E-6
#define GLC_POINT_SIZE		128

typedef struct {
  GLint code;
  GLCchar* name;
} __glcDataCodeFromName;

typedef void* (*__glcProcessCharFunc)(GLint inCode, GLint inFont,
				      __glcContextState* inState,
				      void* inProcessCharData,
				      GLboolean inMultipleChars);

/* Character renderers */
extern void __glcRenderCharScalable(__glcFont* inFont,
				    __glcContextState* inState, GLint inCode,
				    GLCenum inRenderMode,
				    GLboolean inDisplayListIsBuilding,
				    GLfloat* inTransformMatrix,
				    GLfloat scale_x, GLfloat scale_y,
				    __glcCharMapEntry* inCharMapEntry);

/* QuesoGLC own memory management routines */
extern void* __glcMalloc(size_t size);
extern void __glcFree(void* ptr);
extern void* __glcRealloc(void* ptr, size_t size);

/* Find a token in a list of tokens separated by 'separator' */
extern GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex,
				   const GLCchar* inSeparator);

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

/* Duplicate a UTF-8 string to another format */
extern GLCchar* __glcConvertFromUtf8(const FcChar8* inString,
				     const GLint inStringType);

/* Duplicate a UTF-8 string to the context buffer */
extern GLCchar* __glcConvertFromUtf8ToBuffer(__glcContextState* This,
					     const FcChar8* inString,
					     const GLint inStringType);

/* Duplicate a counted string in UTF-8 format */
FcChar8* __glcConvertCountedStringToUtf8(const GLint inCount,
					 const GLCchar* inString,
					 const GLint inStringType);

/* Count the number of bits that are set in c1  */
extern FcChar32 __glcCharSetPopCount(FcChar32 c1);

/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. This function is needed since the GLC specs store
 * individual character codes in GLint which may cause problems for the UTF-8
 * format.
 */
GLint __glcConvertUcs4ToGLint(__glcContextState *inState, GLint inCode);

/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character
 * codes in GLint which may cause problems for the UTF-8 format.
 */
GLint __glcConvertGLintToUcs4(__glcContextState *inState, GLint inCode);

/* Verify that the thread has a current context and that the master identified
 * by 'inMaster' exists.
 */
__glcMaster* __glcVerifyMasterParameters(GLint inMaster);

/* Get the minimum mapped code of a character set */
GLint __glcGetMinMappedCode(FcCharSet *charSet);

/* Get the maximum mapped code of a character set */
GLint __glcGetMaxMappedCode(FcCharSet *charSet);

/* Get the name of the character at offset inIndex from the first element of
 * the character set inCharSet
 */
GLCchar* __glcGetCharNameByIndex(FcCharSet* inCharSet, GLint inIndex,
				 __glcContextState* inState);

/* Return a struct which contains thread specific info */
threadArea* __glcGetThreadArea(void);

/* Raise an error */
void __glcRaiseError(GLCenum inError);

/* Return the current context state */
__glcContextState* __glcGetCurrent(void);

/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code and '<hexcode>' format
 * are issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__glcContextState *inState, GLint inCode,
		       __glcProcessCharFunc inProcessCharFunc,
		       void* inProcessCharData);

/* Load the glyph that correspond to the Unicode codepoint inCode and determine
 * an optimal size for that glyph to be rendered on the screen if no display
 * list is planned to be built.
 */
__glcFont* __glcLoadAndScaleGlyph(__glcContextState* inState, GLint inFont,
				  GLint inCode, GLfloat* outTransformMatrix,
				  GLfloat* outScaleX, GLfloat* outScaleY,
				  GLboolean* outDisplayListIsBuilding,
				  __glcCharMapEntry** outCharMapEntry);
#endif /* __glc_internal_h */
