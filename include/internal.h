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
 *  header of all private functions that are used throughout the library.
 */

#ifndef __glc_internal_h
#define __glc_internal_h

#include <stdlib.h>

#ifndef DEBUGMODE
#define NDEBUG
#endif
#include <assert.h>

#include "GL/glew.h"
#define GLCAPI GLEWAPI
#include "GL/glc.h"
#ifdef HAVE_CONFIG_H
#include "qglc_config.h"
#endif

#define GLCchar8  FcChar8
#define GLCchar32 FcChar32
#define GLCuint   FT_UInt
#define GLClong   FT_Long
#define GLCulong  FT_ULong

#include "ofont.h"

#define GLC_OUT_OF_RANGE_LEN	11
#define GLC_EPSILON		1E-6
#define GLC_POINT_SIZE		128

#if defined(__GNUC__)
# define GLC_UNUSED_ARG(_arg) GLC_UNUSED_ ## _arg __attribute__((unused))
#elif defined(__LCLINT__)
# define GLC_UNUSED_ARG(_arg) /*@unused@*/ GLC_UNUSED_ ## _arg
#else
# define GLC_UNUSED_ARG(_arg) GLC_UNUSED_ ## _arg
#endif

/* Definition of the GLC_INIT_THREAD macro : it is some sort of an equivalent to
 * XInitThreads(). It allows to get rid of pthread_get_specific()/TlsGetValue()
 * when only one thread is used and to fallback to the usual thread management
 * if more than one thread is used.
 * If Thread Local Storage is used the macro does nothing.
 */
#ifdef __WIN32__
# define GLC_INIT_THREAD() \
  if (!InterlockedCompareExchange(&__glcCommonArea.__glcInitThreadOnce, 1, 0)) \
    __glcInitThread();
#elif !defined(HAVE_TLS)
# define GLC_INIT_THREAD() \
  pthread_once(&__glcCommonArea.__glcInitThreadOnce, __glcInitThread);
#else
#define GLC_INIT_THREAD()
#endif

/* Definition of the GLC_GET_THREAD_AREA macro */
#ifdef __WIN32__
# define GLC_GET_THREAD_AREA() \
  (((__glcCommonArea.threadID == GetCurrentThreadId()) && __glcThreadArea) ? \
    __glcThreadArea : __glcGetThreadArea())
#elif !defined(HAVE_TLS)
# define GLC_GET_THREAD_AREA() \
  ((pthread_equal(__glcCommonArea.threadID, pthread_self()) && __glcThreadArea) ? \
    __glcThreadArea : __glcGetThreadArea())
#else
# define GLC_GET_THREAD_AREA() &__glcTlsThreadArea
#endif

/* Definition of the GLC_GET_CURRENT_CONTEXT macro */
#ifdef __WIN32__
# define GLC_GET_CURRENT_CONTEXT() \
  (((__glcCommonArea.threadID == GetCurrentThreadId()) && __glcThreadArea) ? \
    __glcThreadArea->currentContext : __glcGetCurrent())
#elif !defined(HAVE_TLS)
# define GLC_GET_CURRENT_CONTEXT() \
  ((pthread_equal(__glcCommonArea.threadID, pthread_self()) && __glcThreadArea) ? \
    __glcThreadArea->currentContext : __glcGetCurrent())
#else
#define GLC_GET_CURRENT_CONTEXT() __glcTlsThreadArea.currentContext
#endif

typedef struct __GLCdataCodeFromNameRec __GLCdataCodeFromName;
typedef struct __GLCgeomBatchRec __GLCgeomBatch;
typedef struct __GLCcharacterRec __GLCcharacter;

struct __GLCdataCodeFromNameRec {
  GLint code;
  GLCchar* name;
};

struct __GLCgeomBatchRec {
  GLenum mode;
  GLint length;
  GLuint start;
  GLuint end;
};

struct __GLCcharacterRec {
  GLint code;
  __GLCfont* font;
};

/* Callback function type that is called by __glcProcessChar().
 * It allows to unify the character processing before the rendering or the
 * measurement of a character : __glcProcessChar() is called first (see below)
 * then the callback function of type __glcProcessCharFunc is called by
 * __glcProcessChar(). Two functions are defined according to this type :
 * __glcRenderChar() for rendering and __glcGetCharMetric() for measurement.
 */
typedef void* (*__glcProcessCharFunc)(GLint inCode, GLint inPrevCode,
				      GLboolean inIsRTL, __GLCfont* inFont,
				      __GLCcontext* inContext,
				      void* inProcessCharData,
				      GLboolean inMultipleChars);

/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code or the '\<hexcode>'
 * character sequence is issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__GLCcontext *inContext, GLint inCode,
		       __GLCcharacter* inPrevCode, GLboolean inIsRTL,
		       __glcProcessCharFunc inProcessCharFunc,
		       void* inProcessCharData);

/* Render scalable characters using either the GLC_LINE style or the
 * GLC_TRIANGLE style
 */
extern void __glcRenderCharScalable(__GLCfont* inFont, __GLCcontext* inContext,
				    GLCenum inRenderMode,
				    GLfloat* inTransformMatrix,
				    GLfloat scale_x, GLfloat scale_y,
				    __GLCglyph* inGlyph);

/* QuesoGLC own memory management routines */
extern void* __glcMalloc(size_t size);
extern void __glcFree(void* ptr);
extern void* __glcRealloc(void* ptr, size_t size);

/* Find a token in a list of tokens separated by 'separator' */
extern GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex,
				   const GLCchar* inSeparator);

/* Arrays that contain the Unicode name of characters */
extern const __GLCdataCodeFromName __glcCodeFromNameArray[];
extern const GLint __glcNameFromCodeArray[];
extern const GLint __glcMaxCode;
extern const GLint __glcCodeFromNameSize;

/* Find a Unicode name from its code */
extern GLCchar* __glcNameFromCode(GLint code);

/* Find a Unicode code from its name */
extern GLint __glcCodeFromName(GLCchar* name);

/* Duplicate a string and convert if from any Unicode format to UTF-8 format */
extern GLCchar8* __glcConvertToUtf8(const GLCchar* inString,
				    const GLint inStringType);

/* Duplicate a string to the context buffer and convert it from UTF-8 format to
 * any Unicode format.
 */
extern GLCchar* __glcConvertFromUtf8ToBuffer(__GLCcontext* This,
					     const GLCchar8* inString,
					     const GLint inStringType);

/* Duplicate a counted string to the context buffer and convert it from any
 * Unicode format to UTF-8 format.
 */
GLCchar8* __glcConvertCountedStringToUtf8(const GLint inCount,
					  const GLCchar* inString,
					  const GLint inStringType);

/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. This function is needed since the GLC specs store
 * individual character codes in GLint whatever is their string type.
 */
GLint __glcConvertUcs4ToGLint(__GLCcontext *inContext, GLint inCode);

/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character
 * codes in GLint whatever is their string type.
 */
GLint __glcConvertGLintToUcs4(__GLCcontext *inContext, GLint inCode);

/* Verify that the thread has a current context and that the master identified
 * by 'inMaster' exists. Returns a FontConfig pattern corresponding to the
 * master ID 'inMaster'.
 */
FcPattern* __glcVerifyMasterParameters(GLint inMaster);

/* Verify that the thread has a current context and that the font identified
 * by 'inFont' exists.
 */
__GLCfont* __glcVerifyFontParameters(GLint inFont);

/* Do the actual job of glcAppendFont(). This function can be called as an
 * internal version of glcAppendFont() where the current GLC context is already
 * determined and the font ID has been resolved in its corresponding __GLCfont
 * object.
 */
void __glcAppendFont(__GLCcontext* inContext, __GLCfont* inFont);

/* This internal function deletes the font identified by inFont (if any) and
 * creates a new font based on the pattern 'inPattern'. The resulting font is
 * added to the list GLC_FONT_LIST.
 */
__GLCfont* __glcNewFontFromMaster(__GLCfont* inFont, GLint inFontID,
				  FcPattern* inPattern,
				  __GLCcontext *inContext);

/* This internal function tries to open the face file which name is identified
 * by 'inFace'. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __GLCfont object "inFont". Otherwise, it leaves the
 * font unchanged. GL_TRUE or GL_FALSE are returned to indicate if the function
 * succeeded or not.
 */
GLboolean __glcFontFace(__GLCfont* inFont, const GLCchar8* inFace,
			__GLCcontext *inContext);

#ifndef HAVE_TLS
/* Return a struct which contains thread specific info. If the platform supports
 * pointers for thread-local storage (TLS) then __glcGetThreadArea is replaced
 * by a macro that returns a thread-local pointer. Otherwise, a function is
 * called to return the structure using pthread_get_specific (POSIX) or
 * TlsGetValue (WIN32) which are much slower.
 */
__GLCthreadArea* __glcGetThreadArea(void);
#endif

/* Raise an error.
 * See also remarks above about TLS pointers.
 */
#ifdef HAVE_TLS
#define __glcRaiseError(inError) \
if (!__glcTlsThreadArea.errorState || ! (inError)) \
  __glcTlsThreadArea.errorState = (inError)
#else
void __glcRaiseError(GLCenum inError);
#endif

#ifndef HAVE_TLS
/* Return the current context state.
 * See also remarks above about TLS pointers.
 */
__GLCcontext* __glcGetCurrent(void);
#endif

/* Compute an optimal size for the glyph to be rendered on the screen (if no
 * display list is currently building).
 */
void __glcGetScale(__GLCcontext* inContext, GLfloat* outTransformMatrix,
		   GLfloat* outScaleX, GLfloat* outScaleY);

/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
GLCchar32* __glcConvertToVisualUcs4(__GLCcontext* inContext, GLboolean *outIsRTL,
				    const GLCchar* inString);

/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
GLCchar32* __glcConvertCountedStringToVisualUcs4(__GLCcontext* inContext,
						 GLboolean *outIsRTL,
						 const GLCchar* inString,
						 const GLint inCount);

#ifdef FT_CACHE_H
/* Callback function used by the FreeType cache manager to open a given face */
FT_Error __glcFileOpen(FTC_FaceID inFile, FT_Library inLibrary,
		       FT_Pointer inData, FT_Face* outFace);

/* Rename FTC_Manager_LookupFace for old freetype versions */
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
#   define FTC_Manager_LookupFace FTC_Manager_Lookup_Face
# endif
#endif

/* Save the GL State in a structure */
void __glcSaveGLState(__GLCglState* inGLState, __GLCcontext* inContext,
		      GLboolean inAll);

/* Restore the GL State from a structure */
void __glcRestoreGLState(__GLCglState* inGLState, __GLCcontext* inContext,
			 GLboolean inAll);

/* Get a FontConfig pattern from a master ID */
FcPattern* __glcGetPatternFromMasterID(GLint inMaster,
				       __GLCcontext* inContext);

/* Get a face descriptor object from a FontConfig pattern */
__GLCfaceDescriptor* __glcGetFaceDescFromPattern(FcPattern* inPattern,
						 __GLCcontext* inContext);

/* Initialize the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
void __glcCreateHashTable(__GLCcontext *inContext);

/* Update the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
void __glcUpdateHashTable(__GLCcontext *inContext);

/* Function for GLEW so that it can get a context */
GLEWContext* glewGetContext(void);

#ifndef HAVE_TLS
/* This function initializes the thread management of QuesoGLC when TLS is not
 * available. It must be called once (see the macro GLC_INIT_THREAD)
 */
void __glcInitThread(void);
#endif

#endif /* __glc_internal_h */
