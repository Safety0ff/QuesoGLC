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
 * defines the routines used to render characters with lines and triangles.
 */

#include "internal.h"

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <math.h>

#include FT_OUTLINE_H
#include FT_LIST_H

#define GLC_MAX_ITER	50

typedef struct __GLCrendererDataRec __GLCrendererData;

struct __GLCrendererDataRec {
  FT_Vector pen;			/* Current coordinates */
  GLfloat tolerance;			/* Chordal tolerance */
  __GLCarray* vertexArray;		/* Array of vertices */
  __GLCarray* controlPoints;		/* Array of control points */
  __GLCarray* endContour;		/* Array of contour limits */
  __GLCarray* vertexIndices;		/* Array of vertex indices */
  __GLCarray* geomBatches;		/* Array of geometric batches */
  GLfloat* transformMatrix;		/* Transformation matrix from the
					   object space to the viewport */
  GLfloat halfWidth;
  GLfloat halfHeight;
  GLboolean displayListIsBuilding;	/* Is a display list planned to be
					   built ? */
};



/* Transform the object coordinates in the array 'inCoord' in screen
 * coordinates. The function updates 'inCoord' according to :
 * inCoord[0..1] contains the 2D glyph coordinates in the object space
 * inCoord[2..4] contains the 2D homogeneous coordinates in observer space
 */
static void __glcComputePixelCoordinates(GLfloat* inCoord,
					 __GLCrendererData* inData)
{
  GLfloat x = inCoord[0] * inData->transformMatrix[0]
	     + inCoord[1] * inData->transformMatrix[4]
	     + inData->transformMatrix[12];
  GLfloat y = inCoord[0] * inData->transformMatrix[1]
	     + inCoord[1] * inData->transformMatrix[5]
	     + inData->transformMatrix[13];
  GLfloat w = inCoord[0] * inData->transformMatrix[3]
	     + inCoord[1] * inData->transformMatrix[7]
	     + inData->transformMatrix[15];
  GLfloat norm = x * x + y * y;

  /* If w is very small compared to x, y and z this probably means that the
   * transformation matrix is ill-conditioned (i.e. its determinant is
   * numerically null)
   */
  if (w * w < norm * GLC_EPSILON * GLC_EPSILON) {
    /* Ugly hack to handle the singularity of w */
    w = sqrt(norm) * GLC_EPSILON;
  }

  inCoord[2] = x;
  inCoord[3] = y;
  inCoord[4] = w;
}



/* __glcdeCasteljauConic :
 *   renders conic Bezier curves using the de Casteljau subdivision algorithm
 *
 * This function creates a piecewise linear curve which is close enough
 * to the real Bezier curve. The piecewise linear curve is built so that
 * the chordal distance is lower than a tolerance value.
 * The chordal distance is taken to be the perpendicular distance from each
 * control point to the chord. This may not always be correct, but, in the small
 * lengths which are being considered, this is good enough.
 * A second simplifying assumption is that when too large a chordal distance is
 * encountered, the chord is split at the parametric midpoint, rather than
 * guessing the exact location of the best chord. This could lead to slightly
 * sub-optimal lines, but it provides a fast method for choosing the
 * subdivision point. This guess can be refined by lengthening the lines.
 */ 
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcdeCasteljauConic(const FT_Vector **inVector, void *inUserData)
#else
static int __glcdeCasteljauConic(FT_Vector **inVector, void *inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  GLfloat(*controlPoint)[5] = NULL;
  GLint nArc = 1, arc = 0, rank = 0;
  int iter = 0;
  GLfloat* cp = (GLfloat*)__glcArrayInsertCell(data->controlPoints,
			GLC_ARRAY_LENGTH(data->controlPoints), 3);

  if (!cp) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }

  /* Append the control points to the vertex array */
  cp[0] = (GLfloat) data->pen.x;
  cp[1] = (GLfloat) data->pen.y;
  __glcComputePixelCoordinates(cp, data);

  /* Append the first vertex of the curve to the vertex array */
  rank = GLC_ARRAY_LENGTH(data->vertexArray);
  if (!__glcArrayAppend(data->vertexArray, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }

  /* Build the array of the control points */
  for (iter = 0; iter < 2; iter++) {
    cp += 5;
    cp[0] = (GLfloat) inVector[iter]->x;
    cp[1] = (GLfloat) inVector[iter]->y;
    __glcComputePixelCoordinates(cp, data);
  }

  /* controlPoint[] must be computed there because
   * GLC_ARRAY_DATA(data->controlPoints) may have been modified by a realloc()
   * in __glcArrayInsert().
   */
  controlPoint = (GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints);

  /* Here the de Casteljau algorithm begins */
  for (iter = 0; (iter < GLC_MAX_ITER) && (arc != nArc); iter++) {
    GLfloat ax = controlPoint[0][2];
    GLfloat ay = controlPoint[0][3];
    GLfloat aw = controlPoint[0][4];
    GLfloat abx = controlPoint[2][2]*aw - ax*controlPoint[2][4];
    GLfloat aby = controlPoint[2][3]*aw - ay*controlPoint[2][4];
    /* For the middle control point, compute its chordal distance that is its
     * distance from the segment AB
     */
    GLfloat mw = controlPoint[1][4];
    GLfloat s = ((controlPoint[1][2]*aw - ax*mw) * aby
	      -  (controlPoint[1][3]*aw - ay*mw) * abx)
	      / (aw * mw);
    GLfloat dmax = s * s;


    if (dmax < data->tolerance * (abx * abx + aby *aby)) {
      arc++; /* Process the next arc */
      controlPoint = ((GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints))+2*arc;
      /* Update the place where new vertices will be inserted in the vertex
       * array
       */
      rank++;
    }
    else {
      /* Split an arc into two smaller arcs (this is the actual de Casteljau
       * algorithm)
       */
      GLfloat *p1, *p2;
      GLfloat *pm = (GLfloat*)__glcArrayInsertCell(data->controlPoints,
						   2*arc+1, 2);

      if (!pm) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	return 1;
      }

      /* controlPoint[] must be updated there because
       * data->controlPoints->data may have been modified by a realloc() in
       * __glcArrayInsert()
       */
      controlPoint = ((GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints))+2*arc;

      p1 = controlPoint[0];
      p2 = controlPoint[3];
      pm = controlPoint[1];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      p1 = controlPoint[3];
      p2 = controlPoint[4];
      pm = controlPoint[3];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      p1 = controlPoint[1];
      p2 = controlPoint[3];
      pm = controlPoint[2];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      /* The point in pm[] is a point located on the Bezier curve : it must be
       * added to the vertex array
       */
      if (!__glcArrayInsert(data->vertexArray, rank+1, pm)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	return 1;
      }

      nArc++; /* A new arc has been defined */
    }
  }

  /* The array of control points must be emptied in order to be ready for the
   * next call to the de Casteljau routine
   */
  GLC_ARRAY_LENGTH(data->controlPoints) = 0;

  return 0;
}



/* __glcdeCasteljauCubic :
 *   renders cubic Bezier curves using the de Casteljau subdivision algorithm
 *
 * See also remarks about __glcdeCasteljauConic.
 */ 
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcdeCasteljauCubic(const FT_Vector **inVector, void *inUserData)
#else
static int __glcdeCasteljauCubic(FT_Vector **inVector, void *inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  GLfloat(*controlPoint)[5] = NULL;
  GLint nArc = 1, arc = 0, rank = 0;
  int iter = 0;
  GLfloat* cp = (GLfloat*)__glcArrayInsertCell(data->controlPoints,
			GLC_ARRAY_LENGTH(data->controlPoints), 4);

  if (!cp) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }


  /* Append the control points to the vertex array */
  cp[0] = (GLfloat) data->pen.x;
  cp[1] = (GLfloat) data->pen.y;
  __glcComputePixelCoordinates(cp, data);

  /* Append the first vertex of the curve to the vertex array */
  rank = GLC_ARRAY_LENGTH(data->vertexArray);
  if (!__glcArrayAppend(data->vertexArray, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }

  /* Build the array of the control points */
  for (iter = 0; iter < 3; iter++) {
    cp += 5;
    cp[0] = (GLfloat) inVector[iter]->x;
    cp[1] = (GLfloat) inVector[iter]->y;
    __glcComputePixelCoordinates(cp, data);
  }

  /* controlPoint[] must be computed there because data->controlPoints->data
   * may have been modified by a realloc() in __glcArrayInsert()
   */
  controlPoint = (GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints);

  /* Here the de Casteljau algorithm begins */
  for (iter = 0; (iter < GLC_MAX_ITER) && (arc != nArc); iter++) {
    GLfloat ax = controlPoint[0][2];
    GLfloat ay = controlPoint[0][3];
    GLfloat aw = controlPoint[0][4];
    GLfloat abx = controlPoint[3][2]*aw - ax*controlPoint[3][4];
    GLfloat aby = controlPoint[3][3]*aw - ay*controlPoint[3][4];
    /* For the middle control point, compute its chordal distance that is its
     * distance from the segment AB
     */
    GLfloat mw = controlPoint[1][4];
    GLfloat s = ((controlPoint[1][2]*aw - ax*mw) * aby
	      -  (controlPoint[1][3]*aw - ay*mw) * abx)
	      / (aw * mw);
    GLfloat dmax = s * s;
    GLfloat d;

    mw = controlPoint[2][4];
    s = ((controlPoint[2][2]*aw - ax*mw) * aby
      -  (controlPoint[2][3]*aw - ay*mw) * abx)
      / (aw * mw);
    d = s * s;

    dmax = d > dmax ? d : dmax;

    if (dmax < data->tolerance * (abx * abx + aby *aby)) {
      arc++; /* Process the next arc */
      controlPoint = ((GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints))+3*arc;
      /* Update the place where new vertices will be inserted in the vertex
       * array
       */
      rank++;
    }
    else {
      /* Split an arc into two smaller arcs (this is the actual de Casteljau
       * algorithm)
       */
      GLfloat *p1, *p2, *p3;
      GLfloat *pm = (GLfloat*)__glcArrayInsertCell(data->controlPoints,
						   3*arc+1, 3);

      if (!pm) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	return 1;
      }

      /* controlPoint[] must be updated there because
       * data->controlPoints->data may have been modified by a realloc() in
       * __glcArrayInsert()
       */
      controlPoint = ((GLfloat(*)[5])GLC_ARRAY_DATA(data->controlPoints))+3*arc;

      p1 = controlPoint[0];
      p2 = controlPoint[4];
      pm = controlPoint[1];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      p3 = controlPoint[5];
      pm = controlPoint[2];

      pm[0] = 0.25*(p1[0]+2*p2[0]+p3[0]);
      pm[1] = 0.25*(p1[1]+2*p2[1]+p3[1]);
      pm[2] = 0.25*(p1[2]+2*p2[2]+p3[2]);
      pm[3] = 0.25*(p1[3]+2*p2[3]+p3[3]);
      pm[4] = 0.25*(p1[4]+2*p2[4]+p3[4]);

      p1 = controlPoint[6];
      p2 = controlPoint[5];
      pm = controlPoint[5];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      p1 = controlPoint[4];
      p2 = controlPoint[5];
      p3 = controlPoint[6];
      pm = controlPoint[4];

      pm[0] = 0.25*(p1[0]+4*p2[0]-p3[0]);
      pm[1] = 0.25*(p1[1]+4*p2[1]-p3[1]);
      pm[2] = 0.25*(p1[2]+4*p2[2]-p3[2]);
      pm[3] = 0.25*(p1[3]+4*p2[3]-p3[3]);
      pm[4] = 0.25*(p1[4]+4*p2[4]-p3[4]);

      p1 = controlPoint[2];
      p2 = controlPoint[4];
      pm = controlPoint[3];

      pm[0] = 0.5*(p1[0]+p2[0]);
      pm[1] = 0.5*(p1[1]+p2[1]);
      pm[2] = 0.5*(p1[2]+p2[2]);
      pm[3] = 0.5*(p1[3]+p2[3]);
      pm[4] = 0.5*(p1[4]+p2[4]);

      /* The point in pm[] is a point located on the Bezier curve : it must be
       * added to the vertex array
       */
      if (!__glcArrayInsert(data->vertexArray, rank+1, pm)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	return 1;
      }

      nArc++; /* A new arc has been defined */
    }
  }

  /* The array of control points must be emptied in order to be ready for the
   * next call to the de Casteljau routine
   */
  GLC_ARRAY_LENGTH(data->controlPoints) = 0;

  return 0;
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

  data->pen = *inVecTo;
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
  GLfloat vertex[2];

  vertex[0] = (GLfloat) data->pen.x;
  vertex[1] = (GLfloat) data->pen.y;
  if (!__glcArrayAppend(data->vertexArray, vertex)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  data->pen = *inVecTo;
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
  const FT_Vector* vector[2];
#else
static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo,
			void* inUserData)
{
  FT_Vector* vector[2];
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  vector[0] = inVecControl;
  vector[1] = inVecTo;
  error = __glcdeCasteljauConic(vector, inUserData);
  data->pen = *inVecTo;

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
  const FT_Vector* vector[3];
#else
static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2,
			FT_Vector *inVecTo, void* inUserData)
{
  FT_Vector* vector[3];
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  vector[0] = inVecControl1;
  vector[1] = inVecControl2;
  vector[2] = inVecTo;
  error = __glcdeCasteljauCubic(vector, inUserData);
  data->pen = *inVecTo;

  return error;
}



/* Callback function that is called by the GLU when it is tesselating a
 * polygon.
 */
static void CALLBACK __glcCombineCallback(GLdouble coords[3],
				 void* GLC_UNUSED_ARG(vertex_data[4]),
                                 GLfloat GLC_UNUSED_ARG(weight[4]),
				 void** outData, void* inUserData)
{
  __GLCrendererData *data = (__GLCrendererData*)inUserData;
  GLfloat vertex[2];
  /* Evil hack for 32/64 bits compatibility */
  union {
    void* ptr;
    GLuint i;
  } uintInPtr;

  /* Compute the new vertex and append it to the vertex array */
  vertex[0] = (GLfloat)coords[0];
  vertex[1] = (GLfloat)coords[1];
  if (!__glcArrayAppend(data->vertexArray, vertex)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Returns the index of the new vertex in the vertex array */
  uintInPtr.i = GLC_ARRAY_LENGTH(data->vertexArray)-1;
  *outData = uintInPtr.ptr;
}



/* Callback function that is called by the GLU when it is rendering the
 * tesselated polygon. This function is needed to convert the indices of the
 * vertex array into the coordinates of the vertex.
 */
static void CALLBACK __glcVertexCallback(void* vertex_data, void* inUserData)
{
  __GLCrendererData *data = (__GLCrendererData*)inUserData;
  __GLCgeomBatch *geomBatch =
			((__GLCgeomBatch*)GLC_ARRAY_DATA(data->geomBatches));
  /* Evil hack for 32/64 bits compatibility */
  union {
    void* ptr;
    GLuint i;
  } uintInPtr;

  geomBatch += GLC_ARRAY_LENGTH(data->geomBatches) - 1;

  uintInPtr.ptr = vertex_data;
  geomBatch->start = (uintInPtr.i < geomBatch->start) ? uintInPtr.i :
							geomBatch->start;
  geomBatch->end = (uintInPtr.i > geomBatch->end) ? uintInPtr.i :
						    geomBatch->end;
  if (!__glcArrayAppend(data->vertexIndices, &uintInPtr.i)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
  geomBatch->length++;
}



static void CALLBACK __glcBeginCallback(GLenum mode, void* inUserData)
{
  __GLCrendererData *data = (__GLCrendererData*)inUserData;
  __GLCgeomBatch geomBatch;

  geomBatch.mode = mode;
  geomBatch.length = 0;
  geomBatch.start = 0xffffffff;
  geomBatch.end = 0;

  if (!__glcArrayAppend(data->geomBatches, &geomBatch)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }
}



/* Callback function that is called by the GLU whenever an error occur during
 * the tesselation of the polygon.
 */
static void CALLBACK __glcCallbackError(GLenum GLC_UNUSED_ARG(inErrorCode))
{
  __glcRaiseError(GLC_RESOURCE_ERROR);
}



/* Function called by __glcRenderChar() and that performs the actual rendering
 * for the GLC_LINE and the GLC_TRIANGLE types. It transforms the outlines of
 * the glyph in polygon contour. If the rendering type is GLC_LINE then the
 * contour is rendered as is and if the rendering type is GLC_TRIANGLE then the
 * contour defines a polygon that is tesselated in triangles by the GLU library
 * before being rendered.
 */
void __glcRenderCharScalable(__GLCfont* inFont, __GLCcontext* inContext,
			     GLCenum inRenderMode,
			     GLboolean inDisplayListIsBuilding,
			     GLfloat* inTransformMatrix, GLfloat scale_x,
			     GLfloat scale_y,
			     __GLCglyph* inGlyph)
{
  FT_Outline *outline = NULL;
  FT_Outline_Funcs outlineInterface;
  __GLCrendererData rendererData;
  GLfloat identityMatrix[16] = {1., 0., 0., 0., 0., 1., 0., 0., 0., 0.,
				       1., 0., 0., 0., 0., 1.};
  FT_Face face = NULL;
  GLfloat sx64 = 64. * scale_x;
  GLfloat sy64 = 64. * scale_y;
  int index = 0;


#ifndef FT_CACHE_H
  face = inFont->faceDesc->face;
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)inFont->faceDesc,
			     &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
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

  rendererData.vertexArray = inContext->vertexArray;
  rendererData.controlPoints = inContext->controlPoints;
  rendererData.endContour = inContext->endContour;
  rendererData.vertexIndices = inContext->vertexIndices;
  rendererData.geomBatches = inContext->geomBatches;

  rendererData.displayListIsBuilding = inDisplayListIsBuilding;

  /* If no display list is planned to be built then compute distances in pixels
   * otherwise use the object space.
   */
  if (!inDisplayListIsBuilding) {
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    rendererData.halfWidth = viewport[2] * 0.5;
    rendererData.halfHeight = viewport[3] * 0.5;
    rendererData.transformMatrix = inTransformMatrix;
    rendererData.transformMatrix[0] *= rendererData.halfWidth / sx64;
    rendererData.transformMatrix[4] *= rendererData.halfWidth / sy64;
    rendererData.transformMatrix[12] *= rendererData.halfWidth;
    rendererData.transformMatrix[1] *= rendererData.halfHeight / sx64;
    rendererData.transformMatrix[5] *= rendererData.halfHeight / sy64;
    rendererData.transformMatrix[13] *= rendererData.halfHeight;
    rendererData.transformMatrix[2] /= sx64;
    rendererData.transformMatrix[3] /= sx64;
    rendererData.transformMatrix[6] /= sy64;
    rendererData.transformMatrix[7] /= sy64;

#if 0
    rendererData.tolerance = .25; /* Half pixel tolerance */
#else
    rendererData.tolerance = 1.; /* Pixel tolerance */
#endif
  }
  else {
    /* Distances are computed in object space, so is the tolerance of the
     * de Casteljau algorithm.
     */
    rendererData.tolerance = 0.005 * sqrt(scale_x*scale_x + scale_y*scale_y)
      * face->units_per_EM / sx64 / sy64;
    rendererData.halfWidth = 0.5;
    rendererData.halfHeight = 0.5;
    rendererData.transformMatrix = identityMatrix;
    rendererData.transformMatrix[0] /= sx64;
    rendererData.transformMatrix[5] /= sy64;
  }

  /* Parse the outline of the glyph */
  if (FT_Outline_Decompose(outline, &outlineInterface, &rendererData)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inContext->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inContext->endContour) = 0;
    GLC_ARRAY_LENGTH(inContext->vertexIndices) = 0;
    GLC_ARRAY_LENGTH(inContext->geomBatches) = 0;
    return;
  }

  if (!__glcArrayAppend(rendererData.endContour,
			&GLC_ARRAY_LENGTH(rendererData.vertexArray))) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inContext->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inContext->endContour) = 0;
    GLC_ARRAY_LENGTH(inContext->vertexIndices) = 0;
    GLC_ARRAY_LENGTH(inContext->geomBatches) = 0;
    return;
  }

  switch(inRenderMode) {
  case GLC_LINE:
    index = 1;
    break;
  case GLC_TRIANGLE:
    index = inContext->enableState.extrude ? 3 : 2;
    break;
  }

  /* Prepare the display list compilation if needed */
  if (inContext->enableState.glObjects) {
    inGlyph->displayList[index] = glGenLists(1);
    if (!inGlyph->displayList[index]) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      GLC_ARRAY_LENGTH(inContext->vertexArray) = 0;
      GLC_ARRAY_LENGTH(inContext->endContour) = 0;
      GLC_ARRAY_LENGTH(inContext->vertexIndices) = 0;
      GLC_ARRAY_LENGTH(inContext->geomBatches) = 0;
      return;
    }

    glNewList(inGlyph->displayList[index], GL_COMPILE);
  }
  if (inDisplayListIsBuilding)
    glScalef(1./sx64, 1./sy64, 1.);

  if (inRenderMode == GLC_TRIANGLE) {
    /* Tesselate the polygon defined by the contour returned by
     * FT_Outline_Decompose().
     */
    GLUtesselator *tess = gluNewTess();
    GLuint j = 0;
    int i = 0;
    GLuint* endContour = (GLuint*)GLC_ARRAY_DATA(rendererData.endContour);
    GLfloat (*vertexArray)[2] =
      (GLfloat(*)[2])GLC_ARRAY_DATA(rendererData.vertexArray);
    GLdouble coords[3] = {0., 0., 0.};
    GLuint *vertexIndices = NULL;
    __GLCgeomBatch *geomBatch = NULL;

    /* Initialize the GLU tesselator */
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);

    gluTessCallback(tess, GLU_TESS_ERROR,
			(void (CALLBACK *) ())__glcCallbackError);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA,
		    (void (CALLBACK *) ())__glcVertexCallback);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA,
		    (void (CALLBACK *) ())__glcCombineCallback);
    gluTessCallback(tess, GLU_TESS_BEGIN_DATA,
		    (void (CALLBACK *) ())__glcBeginCallback);

    gluTessNormal(tess, 0., 0., 1.);

    /* Define the polygon geometry */
    gluTessBeginPolygon(tess, &rendererData);

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++) {
      /* Evil hack for 32/64 bits compatibility */
      union {
	void* ptr;
	GLuint i;
      } uintInPtr;

      gluTessBeginContour(tess);
      for (j = endContour[i]; j < endContour[i+1]; j++) {
	coords[0] = (GLdouble)vertexArray[j][0];
	coords[1] = (GLdouble)vertexArray[j][1];
	uintInPtr.i = j;
	gluTessVertex(tess, coords, uintInPtr.ptr);
      }
      gluTessEndContour(tess);
    }

    /* Close the polygon and run the tesselation */
    gluTessEndPolygon(tess);

    /* Free memory */
    gluDeleteTess(tess);

    glVertexPointer(2, GL_FLOAT, 0, GLC_ARRAY_DATA(rendererData.vertexArray));

    glNormal3f(0.f, 0.f, 1.f);
    vertexIndices = (GLuint*)GLC_ARRAY_DATA(rendererData.vertexIndices);
    geomBatch = (__GLCgeomBatch*)GLC_ARRAY_DATA(rendererData.geomBatches);

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.geomBatches); i++) {
      glDrawRangeElements(geomBatch[i].mode, geomBatch[i].start,
			  geomBatch[i].end, geomBatch[i].length,
			  GL_UNSIGNED_INT, (void*)vertexIndices);
      vertexIndices += geomBatch[i].length;
    }

    /* For extruded glyphes : render the other side and close the contours */
    if (inContext->enableState.extrude) {
      GLfloat ax = 0.f, bx = 0.f, ay = 0.f, by = 0.f;
      GLfloat nx = 0.f, ny = 0.f, n0x = 0.f, n0y = 0.f, length = 0.f;

      glTranslatef(0.0f, 0.0f, -1.0f);
      glNormal3f(0.f, 0.f, -1.f);

      vertexIndices = (GLuint*)GLC_ARRAY_DATA(rendererData.vertexIndices);

      for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.geomBatches); i++) {
	glDrawRangeElements(geomBatch[i].mode, geomBatch[i].start,
			    geomBatch[i].end, geomBatch[i].length,
			    GL_UNSIGNED_INT, (void*)vertexIndices);
	vertexIndices += geomBatch[i].length;
      }
      glTranslatef(0.0f, 0.0f, 1.0f);

      for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j = endContour[i]; j < endContour[i+1]; j++) {
	  if (j == endContour[i]) {
	    ax = vertexArray[endContour[i+1]-1][0];
	    ay = vertexArray[endContour[i+1]-1][1];
	    bx = vertexArray[j+1][0];
	    by = vertexArray[j+1][1];
	    n0x = ay - by;
	    n0y = bx - ax;
	  }
	  else if (j == (endContour[i+1] - 1)) {
	    ax = vertexArray[j-1][0];
	    ay = vertexArray[j-1][1];
	    bx = vertexArray[endContour[i]][0];
	    by = vertexArray[endContour[i]][1];
	  }
	  else {
	    ax = vertexArray[j-1][0];
	    ay = vertexArray[j-1][1];
	    bx = vertexArray[j+1][0];
	    by = vertexArray[j+1][1];
	  }

	  nx = ay - by;
	  ny = bx - ax;
	  length = sqrt(nx*nx + ny*ny);
	  glNormal3f(nx/length, ny/length, 0.f);
	  glVertex2fv(vertexArray[j]);
	  glVertex3f(vertexArray[j][0], vertexArray[j][1], -1.f);
	}
	length = sqrt(n0x*n0x + n0y*n0y);
	glNormal3f(n0x/length, n0y/length, 0.f);
	glVertex2fv(vertexArray[endContour[i]]);
	glVertex3f(vertexArray[endContour[i]][0], vertexArray[endContour[i]][1],
		   -1.f);
	glEnd();
      }
    }
  }
  else {
    /* For GLC_LINE, there is no need to tesselate. The vertex are contained
     * in an array so we use the OpenGL function glDrawArrays().
     */
    int i = 0;
    int* endContour = (int*)GLC_ARRAY_DATA(rendererData.endContour);

    glNormal3f(0., 0., 1.);
    glVertexPointer(2, GL_FLOAT, 0, GLC_ARRAY_DATA(rendererData.vertexArray));

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++)
      glDrawArrays(GL_LINE_LOOP, endContour[i], endContour[i+1]-endContour[i]);
  }

  /* Take into account the advance of the glyph */
  glTranslatef(face->glyph->advance.x, face->glyph->advance.y, 0.);

  if (inDisplayListIsBuilding)
    glScalef(sx64, sy64, 1.);
  if (inContext->enableState.glObjects) {
    glEndList();
    glCallList(inGlyph->displayList[index]);
  }

  GLC_ARRAY_LENGTH(inContext->vertexArray) = 0;
  GLC_ARRAY_LENGTH(inContext->endContour) = 0;
  GLC_ARRAY_LENGTH(inContext->vertexIndices) = 0;
  GLC_ARRAY_LENGTH(inContext->geomBatches) = 0;
}
