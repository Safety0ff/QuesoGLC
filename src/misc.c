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

/* This file defines miscellaneous utility routines used throughout the lib */

#include <math.h>

#include "internal.h"
#include FT_LIST_H



/* QuesoGLC own allocation and memory management routines */
void* __glcMalloc(size_t size)
{
  return malloc(size);
}

void __glcFree(void *ptr)
{
  free(ptr);
}

void* __glcRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}



/* Find a token in a list of tokens separated by 'separator' */
GLCchar* __glcFindIndexList(const GLCchar* inString, GLuint inIndex,
			    const GLCchar* inSeparator)
{
    GLuint i = 0;
    GLuint occurence = 0;
    char *s = (char *)inString;
    char *sep = (char *)inSeparator;

    if (!inIndex)
	return (GLCchar *)inString;

    /* TODO use Unicode instead of ASCII */
    for (i=0; i<=strlen(s); i++) {
	if (s[i] == *sep)
	    occurence++;
	if (occurence == inIndex) {
	    i++;
	    break;
	}
    }


    return (GLCchar *)&s[i];
}



/* Each thread has to store specific informations so they can be retrieved
 * later. __glcGetThreadArea() returns a struct which contains thread specific
 * info for GLC.
 * If the 'threadArea' of the current thread does not exist, it is created and
 * initialized.
 * IMPORTANT NOTE : __glcGetThreadArea() must never use __glcMalloc() and
 *    __glcFree() since those functions could use the exceptContextStack
 *    before it is initialized.
 */
threadArea* __glcGetThreadArea(void)
{
  threadArea *area = NULL;

#ifdef __WIN32__
  area = (threadArea*)TlsGetValue(__glcCommonArea.threadKey);
#else
  area = (threadArea*)pthread_getspecific(__glcCommonArea.threadKey);
#endif

  if (!area) {
    area = (threadArea*)malloc(sizeof(threadArea));
    if (!area)
      return NULL;

    area->currentContext = NULL;
    area->errorState = GLC_NONE;
    area->lockState = 0;
    area->exceptionStack.head = NULL;
    area->exceptionStack.tail = NULL;
    area->failedTry = GLC_NO_EXC;
#ifdef __WIN32__
	if (!TlsSetValue(__glcCommonArea.threadKey, (LPVOID)area)) {
	  free(area);
	  return NULL;
	}
#else
	pthread_setspecific(__glcCommonArea.threadKey, (void*)area);
#endif
  }

  return area;
}



/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcRaiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  /* An error can only be raised if the current error value is GLC_NONE. 
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = area->errorState;
  if ((inError && !error) || !inError)
    area->errorState = inError;
}



/* Get the current context of the issuing thread */
__glcContextState* __glcGetCurrent(void)
{
  threadArea *area = NULL;

  area = __glcGetThreadArea();
  assert(area);

  return area->currentContext;
}



/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code and '<hexcode>' format
 * are issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__glcContextState *inState, GLint inCode,
		       GLint inPrevCode, __glcProcessCharFunc inProcessCharFunc,
		       void* inProcessCharData)
{
  GLint repCode = 0;
  GLint font = 0;

  /* Get a font that maps inCode */
  font = __glcCtxGetFont(inState, inCode);
  if (font >= 0) {
    /* A font has been found */
    return inProcessCharFunc(inCode, inPrevCode, font, inState,
			     inProcessCharData, GL_FALSE);
  }

  /* __glcCtxGetFont() can not find a font that maps inCode, we then attempt to
   * produce an alternate rendering.
   */

  /* If the variable GLC_REPLACEMENT_CODE is nonzero, and __glcCtxGetFont()
   * finds a font that maps the replacement code, we now render the character
   * that the replacement code is mapped to
   */
  repCode = inState->stringState.replacementCode;
  font = __glcCtxGetFont(inState, repCode);
  if (repCode && font)
    return inProcessCharFunc(repCode, inPrevCode, font, inState,
			     inProcessCharData, GL_FALSE);
  else {
    /* If we get there, we failed to render both the character that inCode maps
     * to and the replacement code. Now, we will try to render the character
     * sequence "\<hexcode>", where '\' is the character REVERSE SOLIDUS 
     * (U+5C), '<' is the character LESS-THAN SIGN (U+3C), '>' is the character
     * GREATER-THAN SIGN (U+3E), and 'hexcode' is inCode represented as a
     * sequence of hexadecimal digits. The sequence has no leading zeros, and
     * alphabetic digits are in upper case. The GLC measurement commands treat
     * the sequence as a single character.
     */
    char buf[10];
    GLint i = 0;
    GLint n = 0;

    /* Check if a font maps to '\', '<' and '>'. */
    if (!__glcCtxGetFont(inState, '\\') || !__glcCtxGetFont(inState, '<')
	|| !__glcCtxGetFont(inState, '>'))
      return NULL;

    /* Check if a font maps hexadecimal digits */
    sprintf(buf,"%X", (int)inCode);
    n = strlen(buf);
    for (i = 0; i < n; i++) {
      if (!__glcCtxGetFont(inState, buf[i]))
	return NULL;
    }

    /* Render the '\<hexcode>' sequence */
    inProcessCharFunc('\\', inPrevCode, __glcCtxGetFont(inState, '\\'), inState,
			   inProcessCharData, GL_FALSE);
    inProcessCharFunc('<', '\\', __glcCtxGetFont(inState, '<'), inState,
			   inProcessCharData, GL_TRUE);
    inPrevCode = '<';
    for (i = 0; i < n; i++) {
      inProcessCharFunc(buf[i], inPrevCode, __glcCtxGetFont(inState, buf[i]),
			inState, inProcessCharData, GL_TRUE);
      inPrevCode = buf[i];
    }
    return inProcessCharFunc('>', inPrevCode, __glcCtxGetFont(inState, '>'),
			     inState, inProcessCharData, GL_TRUE);
  }
}



/* Store an 4x4 identity matrix in 'm' */
static void __glcMakeIdentity(GLfloat* m)
{
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}



/* Invert a 4x4 matrix stored in inMatrix. The result is stored in outMatrix
 * It uses the Gauss-Jordan elimination method
 */
static GLboolean __glcInvertMatrix(GLfloat* inMatrix, GLfloat* outMatrix)
{
  int i, j, k, swap;
  GLfloat t;
  GLfloat temp[4][4];

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      temp[i][j] = inMatrix[i*4+j];
    }
  }
  __glcMakeIdentity(outMatrix);

  for (i = 0; i < 4; i++) {
    /* Look for largest element in column */
    swap = i;
    for (j = i + 1; j < 4; j++) {
      if (fabs(temp[j][i]) > fabs(temp[i][i])) {
	swap = j;
      }
    }

    if (swap != i) {
      /* Swap rows */
      for (k = 0; k < 4; k++) {
	t = temp[i][k];
	temp[i][k] = temp[swap][k];
	temp[swap][k] = t;

	t = outMatrix[i*4+k];
	outMatrix[i*4+k] = outMatrix[swap*4+k];
	outMatrix[swap*4+k] = t;
      }
    }

    if (fabs(temp[i][i]) < GLC_EPSILON) {
      /* No non-zero pivot. The matrix is singular, which shouldn't
       * happen. This means the user gave us a bad matrix.
       */
      return GL_FALSE;
    }

    t = temp[i][i];
    for (k = 0; k < 4; k++) {
      temp[i][k] /= t;
      outMatrix[i*4+k] /= t;
    }
    for (j = 0; j < 4; j++) {
      if (j != i) {
	t = temp[j][i];
	for (k = 0; k < 4; k++) {
	  temp[j][k] -= temp[i][k]*t;
	  outMatrix[j*4+k] -= outMatrix[i*4+k]*t;
	}
      }
    }
  }
  return GL_TRUE;
}



/* Mutiply two 4x4 matrices, the operands are stored in inMatrix1 and inMatrix2
 * The result is stored in outMatrix which can be neither inMatrix1 nor
 * inMatrix2.
 */
static void __glcMultMatrices(GLfloat* inMatrix1, GLfloat* inMatrix2,
			      GLfloat* outMatrix)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      outMatrix[i*4+j] = 
	inMatrix1[i*4+0]*inMatrix2[0*4+j] +
	inMatrix1[i*4+1]*inMatrix2[1*4+j] +
	inMatrix1[i*4+2]*inMatrix2[2*4+j] +
	inMatrix1[i*4+3]*inMatrix2[3*4+j];
    }
  }
}



/* Compute an optimal size for the glyph to be rendered on the screen if no
 * display list is planned to be built.
 */
GLboolean __glcGetScale(__glcContextState* inState,
			GLfloat* outTransformMatrix, GLfloat* outScaleX,
			GLfloat* outScaleY)
{
  GLint listIndex = 0;
  GLboolean displayListIsBuilding = GL_FALSE;
  int i = 0;

  /* Check if a display list is currently building */
  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  displayListIsBuilding = listIndex || inState->enableState.glObjects;

  if (inState->renderState.renderStyle != GLC_BITMAP) {
    /* Compute the matrix that transforms object space coordinates to viewport
     * coordinates. If we plan to use object space coordinates, this matrix is
     * set to identity.
     */
    GLfloat projectionMatrix[16];
    GLfloat modelviewMatrix[16];
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

    __glcMultMatrices(modelviewMatrix, projectionMatrix, outTransformMatrix);

    if ((!displayListIsBuilding) && inState->enableState.hinting) {
      GLfloat rs[16], m[16];
      /* Get the scale factors in each X, Y and Z direction */
      GLfloat sx = sqrt(outTransformMatrix[0] * outTransformMatrix[0]
			+outTransformMatrix[1] * outTransformMatrix[1]
			+outTransformMatrix[2] * outTransformMatrix[2]);
      GLfloat sy = sqrt(outTransformMatrix[4] * outTransformMatrix[4]
			+outTransformMatrix[5] * outTransformMatrix[5]
			+outTransformMatrix[6] * outTransformMatrix[6]);
      GLfloat sz = sqrt(outTransformMatrix[8] * outTransformMatrix[8]
			+outTransformMatrix[9] * outTransformMatrix[9]
			+outTransformMatrix[10] * outTransformMatrix[10]);
      GLfloat x = 0., y = 0.;

      memset(rs, 0, 16 * sizeof(GLfloat));
      rs[15] = 1.;
      for (i = 0; i < 3; i++) {
	rs[0+4*i] = outTransformMatrix[0+4*i] / sx;
	rs[1+4*i] = outTransformMatrix[1+4*i] / sy;
	rs[2+4*i] = outTransformMatrix[2+4*i] / sz;
      }
      if (!__glcInvertMatrix(rs, rs)) {
	*outScaleX = 0.f;
	*outScaleY = 0.f;
	return displayListIsBuilding;
      }

      __glcMultMatrices(rs, outTransformMatrix, m);
      x = ((m[0] + m[12])/(m[3] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[1] + m[13])/(m[3] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleX = sqrt(x*x+y*y);
      x = ((m[4] + m[12])/(m[7] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[5] + m[13])/(m[7] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleY = sqrt(x*x+y*y);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }
  else {
    GLfloat determinant = 0., norm = 0.;
    GLfloat *transform = inState->bitmapMatrix;

    /* Compute the norm of the transformation matrix */
    for (i = 0; i < 4; i++) {
      if (fabsf(transform[i]) > norm)
	norm = fabsf(transform[i]);
    }

    determinant = transform[0] * transform[3] - transform[1] * transform[2];

    /* If the transformation is degenerated, nothing needs to be rendered */
    if (fabsf(determinant) < norm * GLC_EPSILON) {
      *outScaleX = 0.f;
      *outScaleY = 0.f;
      return displayListIsBuilding;
    }

    if (inState->enableState.hinting) {
      *outScaleX = sqrt(transform[0]*transform[0]+transform[1]*transform[1]);
      *outScaleY = sqrt(transform[2]*transform[2]+transform[3]*transform[3]);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }

  return displayListIsBuilding;
}



/* Save the GL State in a structure */
void __glcSaveGLState(__glcGLState* inGLState)
{
  inGLState->texture2D = glIsEnabled(GL_TEXTURE_2D);
  inGLState->blend = glIsEnabled(GL_BLEND);
  glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		&inGLState->textureEnvMode);
  glGetIntegerv(GL_BLEND_SRC, &inGLState->blendSrc);
  glGetIntegerv(GL_BLEND_DST, &inGLState->blendDst);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &inGLState->textureID);
}



/* Restore the GL State from a structure */
void __glcRestoreGLState(__glcGLState* inGLState)
{
  glBindTexture(GL_TEXTURE_2D, inGLState->textureID);
  if (!inGLState->texture2D)
    glDisable(GL_TEXTURE_2D);
  if (!inGLState->blend)
      glDisable(GL_BLEND);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, inGLState->textureEnvMode);
  glBlendFunc(inGLState->blendSrc, inGLState->blendDst);
}



/* Get a FontConfig pattern from a master ID */
FcPattern* __glcGetPatternFromMasterID(GLint inMaster,
				       __glcContextState* inState)
{
  FcChar32 hashValue =
		((FcChar32*)GLC_ARRAY_DATA(inState->masterHashTable))[inMaster];
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;
  FcChar8* parse = NULL;

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
  fontSet = FcFontList(inState->config, pattern, objectSet);
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

  /* Ugly hack to make a copy of the pattern of the found font (otherwise it
   * will be deleted with the font set).
   */
  parse = FcNameUnparse(fontSet->fonts[i]);
  FcFontSetDestroy(fontSet);
  if (!parse) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  pattern = FcNameParse(parse);
  __glcFree(parse);
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  return pattern;
}



/* Get a face descriptor object from a FontConfig pattern */
__glcFaceDescriptor* __glcGetFaceDescFromPattern(FcPattern* inPattern,
						 __glcContextState* inState)
{
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;
  __glcFaceDescriptor* faceDesc = NULL;

  objectSet = FcObjectSetBuild(FC_STYLE, FC_CHARSET, FC_SPACING, FC_FILE,
			       FC_INDEX, FC_OUTLINE, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  fontSet = FcFontList(inState->config, inPattern, objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (i = 0; i < fontSet->nfont; i++) {
    FcChar8 *fileName = NULL;
    FcChar8 *styleName = NULL;
    int fixed = 0;
    FcCharSet *charSet = NULL;
    int index = 0;
    FcBool outline = FcFalse;
    FcResult result = FcResultMatch;
#ifdef DEBUGMODE
    FcChar32 base = 0;
    FcChar32 next = 0;
    FcChar32 map[FC_CHARSET_MAP_SIZE];
#endif

    /* Check whether the glyphs are outlines */
    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (!outline)
      continue;

    /* get the file name */
    result = FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName);
    assert(result != FcResultTypeMismatch);
    /* get the style name */
    result = FcPatternGetString(fontSet->fonts[i], FC_STYLE, 0, &styleName);
    assert(result != FcResultTypeMismatch);
    /* Is this a fixed font ? */
    result = FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
    assert(result != FcResultTypeMismatch);
    /* get the char set */
    result = FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet);
    assert(result != FcResultTypeMismatch);
#ifdef DEBUGMODE
    /* Check that the char set is not empty */
    base = FcCharSetFirstPage(charSet, map, &next);
    assert(base != FC_CHARSET_DONE);
#endif
    /* get the index of the font in font file */
    result = FcPatternGetInteger(fontSet->fonts[i], FC_INDEX, 0, &index);
    assert(result != FcResultTypeMismatch);

    /* Create a face descriptor that will contain the whole description of
     * the face (hence the name).
     */
    faceDesc = __glcFaceDescCreate(styleName, charSet,
				   (fixed != FC_PROPORTIONAL), fileName,
				   index);
    if (!faceDesc) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcFontSetDestroy(fontSet);
      return NULL;
    }

    break;
  }

  FcFontSetDestroy(fontSet);
  return faceDesc;
}



/* Update the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
void __glcUpdateHashTable(__glcContextState *inState)
{
  FcPattern* pattern = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;

  /* Use Fontconfig to get the default font files */
  pattern = FcPatternCreate();
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return;
  }
  fontSet = FcFontList(inState->config, pattern, objectSet);
  FcPatternDestroy(pattern);
  FcObjectSetDestroy(objectSet);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Parse the font set looking for fonts that are not already registered in the
   * hash table.
   */
  for (i = 0; i < fontSet->nfont; i++) {
    FcChar32 hashValue = 0;
    int j = 0;
    int length = GLC_ARRAY_LENGTH(inState->masterHashTable);
    FcChar32* hashTable = (FcChar32*)GLC_ARRAY_DATA(inState->masterHashTable);
    FcBool outline = FcFalse;
    FcResult result = FcResultMatch;

    /* Check whether the glyphs are outlines */
    result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    if (!outline)
      continue;

    /* Check if the font is already registered in the hash table */
    hashValue = FcPatternHash(fontSet->fonts[i]);
    for (j = 0; j < length; j++) {
      if (hashTable[j] == hashValue)
	break;
    }

    /* If the font is already registered then parse the next one */
    if (j != length)
      continue;

    /* Register the font (i.e. append its hash value to the hash table) */
    if (!__glcArrayAppend(inState->masterHashTable, &hashValue)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcFontSetDestroy(fontSet);
      return;
    }
  }

  FcFontSetDestroy(fontSet);
}



/* Initialize the hash table that allows to convert master IDs into FontConfig
 * patterns.
 */
void __glcCreateHashTable(__glcContextState* inState)
{
  /* Empties the hash table */
  GLC_ARRAY_LENGTH(inState->masterHashTable) = 0;
  /* Update the hash table */
  __glcUpdateHashTable(inState);
}
