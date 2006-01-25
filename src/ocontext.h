/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2006, Bertrand Coconnier
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

#include "GL/glc.h"
#include "constant.h"
#include "omaster.h"

typedef struct {
  FT_ListNodeRec node;

  GLboolean isCurrent;
  GLCchar *buffer;
  GLint bufferSize;

  FT_Library library;
  GLint id;			/* Context ID */
  GLboolean pendingDelete;	/* Is there a pending deletion ? */
  GLCfunc callback;		/* Callback function */
  GLvoid* dataPointer;		/* GLC_DATA_POINTER */
  GLboolean autoFont;		/* GLC_AUTO_FONT */
  GLboolean glObjects;		/* GLC_GLOBJECTS */
  GLboolean mipmap;		/* GLC_MIPMAP */
  GLfloat resolution;		/* GLC_RESOLUTION */
  GLfloat bitmapMatrix[4];	/* GLC_BITMAP_MATRIX */
  FT_ListRec currentFontList;	/* GLC_CURRENT_FONT_LIST */
  FT_ListRec fontList;		/* GLC_FONT_LIST */
  FT_ListRec masterList;
  FcStrSet* catalogList;	/* GLC_CATALOG_LIST */
  GLint measuredCharCount;	/* GLC_MEASURED_CHAR_COUNT */
  GLint renderStyle;		/* GLC_RENDER_STYLE */
  GLint replacementCode;	/* GLC_REPLACEMENT_CODE */
  GLint stringType;		/* GLC_STRING_TYPE */
  GLfloat (*measurementCharBuffer)[12];
  GLint measurementCharLength;
  GLfloat measurementStringBuffer[12];
  GLboolean isInCallbackFunc;
  GLint lastFontID;
} __glcContextState;

typedef struct {
  __glcContextState* currentContext;
  GLCenum errorState;
  GLint lockState;
  FT_ListRec exceptContextStack;
} threadArea;

typedef struct {
  GLint versionMajor;		/* GLC_VERSION_MAJOR */
  GLint versionMinor;		/* GLC_VERSION_MINOR */

  FT_ListRec stateList;
  pthread_mutex_t mutex;	/* For concurrent accesses to the common area */

  pthread_key_t threadKey;

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
