/* $Id$ */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_GLYPH_H
#include FT_OUTLINE_H

#define GLC_TEXTURE_SIZE 64

/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(__glcFont* inFont, __glcContextState* inState)
{
    FT_Matrix matrix;
    FT_Face face = inFont->face;
    FT_Outline outline = face->glyph->outline;
    FT_BBox boundBox;
    FT_Bitmap pixmap;
    GLint EM_size = face->units_per_EM;

    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed) (inState->bitmapMatrix[0] * 65536. / EM_size);
    matrix.xy = (FT_Fixed) (inState->bitmapMatrix[2] * 65536. / EM_size);
    matrix.yx = (FT_Fixed) (inState->bitmapMatrix[1] * 65536. / EM_size);
    matrix.yy = (FT_Fixed) (inState->bitmapMatrix[3] * 65536. / EM_size);
    
    FT_Outline_Transform(&outline, &matrix);
    FT_Outline_Get_CBox(&outline, &boundBox);
    
    boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
    boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
    boundBox.xMax = (boundBox.xMax + 32) & -64;	/* ceiling(xMax) */
    boundBox.yMax = (boundBox.yMax + 32) & -64;	/* ceiling(yMax) */
    
    pixmap.width = (boundBox.xMax - boundBox.xMin) / 64;
    pixmap.rows = (boundBox.yMax - boundBox.yMin) / 64;
    pixmap.pitch = pixmap.width / 8 + 1;	/* 1 bit / pixel */
    
    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
    pixmap.buffer = (GLubyte *)malloc(pixmap.rows * pixmap.pitch);
    if (!pixmap.buffer) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);
    
    /* Flip the picture */
    pixmap.pitch = - pixmap.pitch;
    
    /* translate the outline to match (0, 0) with the glyph's lower left corner */
    FT_Outline_Translate(&outline, -boundBox.xMin, -boundBox.yMin);
    
    /* render the glyph */
    if (FT_Outline_Get_Bitmap(__glcContextState::library, &outline, &pixmap)) {
	free(pixmap.buffer);
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(pixmap.width, pixmap.rows, -boundBox.xMin / 64., -boundBox.yMin / 64.,
	face->glyph->advance.x * inState->bitmapMatrix[0] / EM_size / 64., 
	face->glyph->advance.x * inState->bitmapMatrix[1] / EM_size / 64.,
	pixmap.buffer);
    
    free(pixmap.buffer);
}

static void destroyDisplayListKey(void *inKey)
{
  free(inKey);
}

static void destroyDisplayListData(void *inData)
{
  glDeleteLists(*((GLuint *)inData), 1);
  free(inData);
}

static int compareDisplayListKeys(void *inKey1, void *inKey2)
{
  __glcDisplayListKey *key1 = (__glcDisplayListKey *)inKey1;
  __glcDisplayListKey *key2 = (__glcDisplayListKey *)inKey2;

  if (key1->face < key2->face)
    return -1;
  if (key1->face > key2->face)
    return 1;

  if (key1->code < key2->code)
    return -1;
  if (key1->code > key2->code)
    return 1;

  if (key1->renderMode < key2->renderMode)
    return -1;
  if (key1->renderMode > key2->renderMode)
    return 1;

  return 0;
}

static void __glcRenderCharTexture(__glcFont* inFont, __glcContextState* inState, GLint inCode)
{
    FT_Matrix matrix;
    FT_Face face = inFont->face;
    FT_Outline outline = face->glyph->outline;
    FT_BBox boundBox;
    FT_Bitmap pixmap;
    GLint EM_size = face->units_per_EM;
    GLuint texture = 0;
    GLfloat width = 0, height = 0;
    GLint format = 0;
    GLuint *list = NULL;
    __glcDisplayListKey *dlKey = NULL;

    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_LUMINANCE, GLC_TEXTURE_SIZE,
	GLC_TEXTURE_SIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &format);
    if (!format) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed) (GLC_TEXTURE_SIZE * 65536. / EM_size);
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = (FT_Fixed) (GLC_TEXTURE_SIZE * 65536. / EM_size);
    
    FT_Outline_Transform(&outline, &matrix);
    FT_Outline_Get_CBox(&outline, &boundBox);
    
    boundBox.xMin = boundBox.xMin & -64;	/* floor(xMin) */
    boundBox.yMin = boundBox.yMin & -64;	/* floor(yMin) */
    boundBox.xMax = (boundBox.xMax + 32) & -64;	/* ceiling(xMax) */
    boundBox.yMax = (boundBox.yMax + 32) & -64;	/* ceiling(yMax) */
    
    width = (GLfloat)((boundBox.xMax - boundBox.xMin) / 64);
    height = (GLfloat)((boundBox.yMax - boundBox.yMin) / 64);
    pixmap.width = GLC_TEXTURE_SIZE;
    pixmap.rows = GLC_TEXTURE_SIZE;
    pixmap.pitch = pixmap.width;		/* 8 bits / pixel */
    
    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_grays;	/* Anti-aliased rendering */
    pixmap.num_grays = 256;
    pixmap.buffer = (GLubyte *)malloc(pixmap.rows * pixmap.pitch);
    if (!pixmap.buffer) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    /* fill the pixmap buffer with the background color */
    memset(pixmap.buffer, 0, pixmap.rows * pixmap.pitch);

    /* Flip the picture */
    pixmap.pitch = - pixmap.pitch;
    
    /* translate the outline to match (0, 0) with the glyph's lower left corner */
    FT_Outline_Translate(&outline, -boundBox.xMin, -boundBox.yMin);
    
    /* render the glyph */
    if (FT_Outline_Get_Bitmap(__glcContextState::library, &outline, &pixmap)) {
	free(pixmap.buffer);
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }

    if ((inState->glObjects)
    && (inState->textureObjectCount <= GLC_MAX_TEXTURE_OBJECT)) {
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	inState->textureObjectList[inState->textureObjectCount] = texture;
	inState->textureObjectCount++;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (inState->mipmap) {
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE8, GLC_TEXTURE_SIZE,
		GLC_TEXTURE_SIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		pixmap.buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, GLC_TEXTURE_SIZE,
		GLC_TEXTURE_SIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
		pixmap.buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    if (inState->glObjects) {
      list = (GLuint *)malloc(sizeof(GLuint));
      if (!list) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	free(pixmap.buffer);
	return;
      }
      dlKey = (__glcDisplayListKey *)malloc(sizeof(__glcDisplayListKey));
      if (!dlKey) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	free(pixmap.buffer);
	free(list);
	return;
      }

      *list = glGenLists(1);
      dlKey->face = inFont->faceID;
      dlKey->code = inCode;
      dlKey->renderMode = 2;

      if (inFont->parent->displayList)
	inFont->parent->displayList = inFont->parent->displayList->insert(dlKey, list);
      else {
	inFont->parent->displayList = new BSTree(dlKey, list, destroyDisplayListKey, 
					  destroyDisplayListData, compareDisplayListKeys);
	if (!inFont->parent->displayList) {
	  __glcContextState::raiseError(GLC_RESOURCE_ERROR);
	  free(pixmap.buffer);
	  free(dlKey);
	  free(list);
	  return;
	}
      }
      glNewList(*list, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, texture);
    }

    glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex2f(boundBox.xMin / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMin / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(width / GLC_TEXTURE_SIZE, 0.);
	glVertex2f(boundBox.xMax / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMin / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(width / GLC_TEXTURE_SIZE, height / GLC_TEXTURE_SIZE);
	glVertex2f(boundBox.xMax / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMax / 64. / GLC_TEXTURE_SIZE);
	glTexCoord2f(0., height / GLC_TEXTURE_SIZE);
	glVertex2f(boundBox.xMin / 64. / GLC_TEXTURE_SIZE,
		boundBox.yMax / 64. / GLC_TEXTURE_SIZE);
    glEnd();
    
    glTranslatef(face->glyph->advance.x * inState->bitmapMatrix[0] / EM_size / 64., 
	face->glyph->advance.x * inState->bitmapMatrix[1] / EM_size / 64., 0.);
    
    if (inState->glObjects) {
      glEndList();
      inState->listObjectCount++;
      glCallList(*list);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    
    free(pixmap.buffer);
}

static GLboolean __glcFindDisplayList(__glcFont *inFont, GLint inCode, GLint renderMode)
{
    __glcDisplayListKey dlKey;
    GLuint *list = NULL;

    if (inFont->parent->displayList) {
      dlKey.face = inFont->faceID;
      dlKey.code = inCode;
      dlKey.renderMode = renderMode;
      list = (GLuint *)inFont->parent->displayList->lookup(&dlKey);
      if (list) {
	glCallList(*list);
	return GL_TRUE;
      }
    }

    return GL_FALSE;
}

static void __glcRenderChar(GLint inCode, GLint inFont)
{
    __glcContextState *state = __glcContextState::getCurrent();
    __glcFont* font = state->fontList[inFont - 1];
    GLint glyphIndex = 0;
    GLint i = 0;
    
    /* TODO : use a dichotomic algo. instead*/
    for (i = 0; i < font->charMapCount; i++) {
	if (inCode == font->charMap[0][i]) {
	    inCode = font->charMap[1][i];
	    break;
	}
    }
    
    if (FT_Set_Char_Size(font->face, 0, 1 << 16, (unsigned int)state->resolution,
	(unsigned int)state->resolution)) {
	__glcContextState::raiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    glyphIndex = FT_Get_Char_Index(font->face, inCode);
    
    if (FT_Load_Glyph(font->face, glyphIndex, FT_LOAD_DEFAULT)) {
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	return;
    }

    switch(state->renderStyle) {
	case GLC_BITMAP:
	    __glcRenderCharBitmap(font, state);
	    return;
	case GLC_TEXTURE:
	  if (!__glcFindDisplayList(font, inCode, 2))
	      __glcRenderCharTexture(font, state, inCode);
	  return;
	case GLC_LINE:
	case GLC_TRIANGLE:
	  if (!__glcFindDisplayList(font, inCode, (state->renderStyle == GLC_TRIANGLE) ? 4 : 3))
	    __glcRenderCharScalable(font, state, inCode, destroyDisplayListKey, destroyDisplayListData, compareDisplayListKeys, (state->renderStyle == GLC_TRIANGLE));
	  return;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
    }
}

void glcRenderChar(GLint inCode)
{
    GLint repCode = 0;
    GLint font = 0;
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    font = state->getFont(inCode);
    if (font) {
	__glcRenderChar(inCode, font);
	return;
    }

    repCode = glcGeti(GLC_REPLACEMENT_CODE);
    font = state->getFont(repCode);
    if (repCode && font) {
	__glcRenderChar(repCode, font);
	return;
    }
    else {
	/* FIXME : use Unicode instead of ASCII */
	char buf[10];
	GLint i = 0;
	GLint n = 0;
	
	if (!state->getFont('\\') || !state->getFont('<') || !state->getFont('>'))
	    return;

	sprintf(buf,"%X", inCode);
	n = strlen(buf);
	for (i = 0; i < n; i++) {
	    if (!state->getFont(buf[i]))
		return;
	}
	
	__glcRenderChar('\\', state->getFont('\\'));
	__glcRenderChar('<', state->getFont('<'));
	for (i = 0; i < n; i++)
	    __glcRenderChar(buf[i], state->getFont(buf[i]));
	__glcRenderChar('>', state->getFont('>'));
    }
}

void glcRenderCountedString(GLint inCount, const GLCchar *inString)
{
    GLint i = 0;
    __glcContextState *state = NULL;
    __glcUniChar UinString;
    
  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  UinString = __glcUniChar(inString, state->stringType);

  for (i = 0; i < inCount; i++)
    glcRenderChar(UinString.index(i));
}

void glcRenderString(const GLCchar *inString)
{
  __glcContextState *state = NULL;
  __glcUniChar UinString;

  state = __glcContextState::getCurrent();
  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  UinString = __glcUniChar(inString, state->stringType);

  glcRenderCountedString(UinString.len(), inString);
}

void glcRenderStyle(GLCenum inStyle)
{
    __glcContextState *state = NULL;

    switch(inStyle) {
	case GLC_BITMAP:
	case GLC_LINE:
	case GLC_TEXTURE:
	case GLC_TRIANGLE:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->renderStyle = inStyle;
    return;
}

void glcReplacementCode(GLint inCode)
{
    __glcContextState *state = NULL;

    switch(inCode) {
	case GLC_REPLACEMENT_CODE:
	    break;
	default:
	    __glcContextState::raiseError(GLC_PARAMETER_ERROR);
	    return;
    }
    
    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->replacementCode = inCode;
    return;
}

void glcResolution(GLfloat inVal)
{
    __glcContextState *state = NULL;

    state = __glcContextState::getCurrent();
    if (!state) {
	__glcContextState::raiseError(GLC_STATE_ERROR);
	return;
    }

    state->resolution = inVal;
    return;
}
