/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
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

#include <pthread.h>
#ifdef __MACOSX__
#include <ndbm.h>
#else
#include <gdbm/ndbm.h>
#endif

#include "GL/glc.h"
#include "constant.h"
#include "ofont.h"
#include "omaster.h"
#include "ostrlst.h"

typedef struct {
  GLboolean isCurrent;
  GLCchar *buffer;
  GLint bufferSize;

  GLuint displayDPIx;
  GLuint displayDPIy;

  FT_Library library;
  GLint id;			/* Context ID */
  GLboolean pendingDelete;	/* Is there a pending deletion ? */
  GLCfunc callback;		/* Callback function */
  GLvoid* dataPointer;	/* GLC_DATA_POINTER */
  GLboolean autoFont;		/* GLC_AUTO_FONT */
  GLboolean glObjects;	/* GLC_GLOBJECTS */
  GLboolean mipmap;		/* GLC_MIPMAP */
  FT_List catalogList; /* GLC_CATALOG_LIST */
  GLfloat resolution;		/* GLC_RESOLUTION */
  GLfloat bitmapMatrix[4];	/* GLC_BITMAP_MATRIX */
  GLint currentFontList[GLC_MAX_CURRENT_FONT];
  GLint currentFontCount;	/* GLC_CURRENT_FONT_COUNT */
  __glcFont* fontList[GLC_MAX_FONT];
  GLint fontCount;		/* GLC_FONT_COUNT */
  GLint listObjectCount;	/* GLC_LIST_OBJECT_COUNT */
  __glcMaster* masterList[GLC_MAX_MASTER];
  GLint masterCount;		/* GLC_MASTER_COUNT */
  GLint measuredCharCount;	/* GLC_MEASURED_CHAR_COUNT */
  GLint renderStyle;		/* GLC_RENDER_STYLE */
  GLint replacementCode;	/* GLC_REPLACEMENT_CODE */
  GLint stringType;		/* GLC_STRING_TYPE */
  GLuint textureObjectList[GLC_MAX_TEXTURE_OBJECT];
  GLint textureObjectCount;	/* GLC_TEXTURE_OBJECT_COUNT */
  GLint versionMajor;		/* GLC_VERSION_MAJOR */
  GLint versionMinor;		/* GLC_VERSION_MINOR */
  GLfloat measurementCharBuffer[GLC_MAX_MEASURE][12];
  GLfloat measurementStringBuffer[12];
  GLboolean isInCallbackFunc;
} __glcContextState;

typedef struct {
  __glcContextState* currentContext;
  GLCenum errorState;
  GLint lockState;
} threadArea;

typedef struct {
  FT_List stateList;
  pthread_mutex_t mutex; /* For concurrent accesses to the stateList array */
  pthread_mutex_t dbMutex; /* For concurrent accesses to the DB */
  pthread_key_t threadKey;

  FT_Memory memoryManager;
  DBM *unidb1, *unidb2;
} commonArea;

#ifdef QUESOGLC_STATIC_LIBRARY
extern pthread_once_t initLibraryOnce;
#endif
extern commonArea *__glcCommonArea;

__glcContextState* __glcCtxCreate(GLint inContext);
void __glcCtxDestroy(__glcContextState *This);
void __glcCtxAddMasters(__glcContextState *This, const GLCchar* catalog,
			GLboolean append);
void __glcCtxRemoveMasters(__glcContextState *This, GLint inIndex);
GLint __glcCtxGetFont(__glcContextState *This, GLint code);
GLCchar* __glcCtxQueryBuffer(__glcContextState *This, int inSize);

__glcContextState* __glcGetState(GLint inContext);
void __glcLock(void);
void __glcUnlock(void);

__glcContextState* __glcGetCurrent(void);
void __glcRaiseError(GLCenum inError);

threadArea* __glcGetThreadArea(void);

#endif /* __glc_ocontext_h */
