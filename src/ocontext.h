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

#ifndef __glc_ocontext_h
#define __glc_ocontext_h

#ifndef __WIN32__
#include <pthread.h>
#else
#include <windows.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef FT_CACHE_H
#include FT_CACHE_H
#endif

#include "GL/glc.h"
#include "oarray.h"
#include "except.h"

#define GLC_MAX_MATRIX_STACK_DEPTH	32
#define GLC_MAX_ATTRIB_STACK_DEPTH	16

typedef struct {
  GLuint id;
  GLsizei width;
  GLsizei heigth;
} __glcTexture;

typedef struct {
  GLboolean autoFont;		/* GLC_AUTO_FONT */
  GLboolean glObjects;		/* GLC_GLOBJECTS */
  GLboolean mipmap;		/* GLC_MIPMAP */
  GLboolean hinting;		/* GLC_HINTING_QSO */
  GLboolean extrude;		/* GLC_EXTRUDE_QSO */
  GLboolean kerning;		/* GLC_KERNING_QSO */
} __glcEnableState;

typedef struct {
  GLfloat resolution;		/* GLC_RESOLUTION */
  GLint renderStyle;		/* GLC_RENDER_STYLE */
} __glcRenderState;

typedef struct {
  GLint replacementCode;	/* GLC_REPLACEMENT_CODE */
  GLint stringType;		/* GLC_STRING_TYPE */
  GLCfunc callback;		/* Callback function GLC_OP_glcUnmappedCode */
  GLvoid* dataPointer;		/* GLC_DATA_POINTER */
} __glcStringState;

typedef struct {
  GLboolean texture2D;
  GLint textureID;
  GLint textureEnvMode;
  GLboolean blend;
  GLint blendSrc;
  GLint blendDst;
  GLboolean vertexArray;
  GLboolean normalArray;
  GLboolean colorArray;
  GLboolean indexArray;
  GLboolean texCoordArray;
  GLboolean edgeFlagArray;
} __glcGLState;

typedef struct {
  GLbitfield attribBits;
  __glcEnableState enableState;
  __glcRenderState renderState;
  __glcStringState stringState;
  __glcGLState glState;
} __glcAttribStackLevel;

typedef struct {
  FT_ListNodeRec node;

  GLboolean isCurrent;
  GLCchar *buffer;
  GLint bufferSize;

  FT_Library library;
#ifdef FT_CACHE_H
  FTC_Manager cache;
#endif
  FcConfig *config;

  GLint id;			/* Context ID */
  GLboolean pendingDelete;	/* Is there a pending deletion ? */
  __glcEnableState enableState;
  __glcRenderState renderState;
  __glcStringState stringState;
  FT_ListRec currentFontList;	/* GLC_CURRENT_FONT_LIST */
  FT_ListRec fontList;		/* GLC_FONT_LIST */
  __glcArray* masterHashTable;
  FcStrSet* catalogList;	/* GLC_CATALOG_LIST */
  __glcArray* measurementBuffer;
  GLfloat measurementStringBuffer[12];
  GLboolean isInCallbackFunc;
  GLint lastFontID;
  __glcArray* vertexArray;	/* Array of vertices */
  __glcArray* controlPoints;	/* Array of control points */
  __glcArray* endContour;	/* Array of contour limits */
  __glcArray* vertexIndices;	/* Array of vertex indices */
  __glcArray* geomBatches;	/* Array of geometric batches */

  GLEWContext glewContext;	/* GLEW context for OpenGL extensions */
  __glcTexture texture;		/* Texture for immediate mode rendering */

  __glcTexture atlas;
  FT_ListRec atlasList;
  int atlasWidth;
  int atlasHeight;
  int atlasCount;

  GLfloat* bitmapMatrix;	/* GLC_BITMAP_MATRIX */
  GLfloat bitmapMatrixStack[4*GLC_MAX_MATRIX_STACK_DEPTH];
  GLint bitmapMatrixStackDepth;

  __glcAttribStackLevel attribStack[GLC_MAX_ATTRIB_STACK_DEPTH];
  GLint attribStackDepth;
} __glcContextState;

typedef struct {
  __glcContextState* currentContext;
  GLCenum errorState;
  GLint lockState;
  FT_ListRec exceptionStack;
  __glcException failedTry;
} threadArea;

typedef struct {
  GLint versionMajor;		/* GLC_VERSION_MAJOR */
  GLint versionMinor;		/* GLC_VERSION_MINOR */

  FT_ListRec stateList;
#ifndef __WIN32__
  pthread_mutex_t mutex;	/* For concurrent accesses to the common
				   area */

  pthread_key_t threadKey;
#else
  CRITICAL_SECTION section;
  DWORD threadKey;
#endif

  /* Evil hack : we use the FT_MemoryRec_ structure definition which is
   * supposed not to be exported by FreeType headers. So this definition may
   * fail if the guys of FreeType decide not to expose FT_MemoryRec_ anymore.
   * However, this has not happened yet so we still rely on FT_MemoryRec_ ...
   */
  struct FT_MemoryRec_ memoryManager;
} commonArea;

extern commonArea __glcCommonArea;

#ifdef QUESOGLC_STATIC_LIBRARY
extern pthread_once_t initLibraryOnce;
#endif

__glcContextState* __glcCtxCreate(GLint inContext);
void __glcCtxDestroy(__glcContextState *This);
void __glcCtxAddMasters(__glcContextState *This, const GLCchar* catalog,
			GLboolean append);
void __glcCtxRemoveMasters(__glcContextState *This, GLint inIndex);
GLint __glcCtxGetFont(__glcContextState *This, GLint code);
GLCchar* __glcCtxQueryBuffer(__glcContextState *This, int inSize);
void __glcAddFontsToContext(__glcContextState *This, FcFontSet *fontSet,
			    GLboolean inAppend);

#endif /* __glc_ocontext_h */
