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

/* Draws characters of scalable fonts */

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <math.h>

#include "internal.h"
#include FT_OUTLINE_H
#include FT_LIST_H

#define GLC_MAX_ITER	50

typedef struct {
  FT_Vector pen;			/* Current coordinates */
  GLfloat tolerance;			/* Chordal tolerance */
  __glcArray* vertexArray;		/* Array of vertices */
  __glcArray* controlPoints;		/* Array of control points */
  __glcArray* endContour;		/* Array of contour limits */
  GLfloat scale_x;			/* Scale to convert grid point.. */
  GLfloat scale_y;			/* ..coordinates into pixels */
  GLfloat* transformMatrix;		/* Transformation matrix from the
					   object space to the viewport */
  GLfloat halfWidth;
  GLfloat halfHeight;
  GLboolean displayListIsBuilding;	/* Is a display list planned to be
					   built ? */
}__glcRendererData;



/* Transform the object coordinates in the array 'inCoord' in screen
 * coordinates. The function updates 'inCoord' according to :
 * inCoord[0..1] contains the 2D glyph coordinates in the object space
 * inCoord[2..4] contains the 2D homogeneous coordinates in observer space
 * inCoord[5..6] contains the viewport coordinates
 */
static GLfloat __glcComputePixelCoordinates(GLfloat* inCoord,
					 __glcRendererData* inData)
{
  GLfloat x = inCoord[0] * inData->transformMatrix[0]
	     + inCoord[1] * inData->transformMatrix[4]
	     + inData->transformMatrix[12];
  GLfloat y = inCoord[0] * inData->transformMatrix[1]
	     + inCoord[1] * inData->transformMatrix[5]
	     + inData->transformMatrix[13];
  GLfloat z = inCoord[0] * inData->transformMatrix[2]
	     + inCoord[1] * inData->transformMatrix[6]
	     + inData->transformMatrix[14];
  GLfloat w = inCoord[0] * inData->transformMatrix[3]
	     + inCoord[1] * inData->transformMatrix[7]
	     + inData->transformMatrix[15];
  GLfloat norm = x * x + y * y + z * z;

  /* If w is very small compared to x, y and z this probably mean that the
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
  inCoord[5] = x / w;
  inCoord[6] = y / w;

  return z / w;
}

/* __glcdeCasteljau : 
 *	renders bezier curves using the de Casteljau subdivision algorithm
 *
 * This function creates a piecewise linear curve which is close enough
 * to the real Bezier curve. The piecewise linear curve is built so that
 * the chordal distance is lower than a tolerance value.
 * The chordal distance is taken to be the perpendicular distance from the
 * parametric midpoint, (t = 0.5), to the chord. This may not always be
 * correct, but, in the small lengths which are being considered, this is good
 * enough. In order to mitigate any error, the chordal tolerance is taken to be
 * slightly smaller than the tolerance specified by the user.
 * A second simplifying assumption is that when too large a tolerance is
 * encountered, the chord is split at the parametric midpoint, rather than
 * guessing the exact location of the best chord. This could lead to slightly
 * sub-optimal lines, but it provides a fast method for choosing the
 * subdivision point. This guess can be refined by lengthening the lines.
 */ 
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcdeCasteljau(const FT_Vector *inVecTo,
			    const FT_Vector **inControl, void *inUserData,
			    GLint inOrder)
#else
static int __glcdeCasteljau(FT_Vector *inVecTo, FT_Vector **inControl,
			    void *inUserData, GLint inOrder)
#endif
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  GLfloat(*controlPoint)[7] = NULL;
  GLfloat cp[7] = {0., 0., 0., 0., 0., 0., 0.};
  GLint i = 0, nArc = 1, arc = 0, rank = 0;
  GLfloat xMin = 0., xMax = 0., yMin =0., yMax = 0., zMin = 0., zMax = 0.;
  GLfloat z = 0.;
  int iter = 0;

  /* Append the control points to the vertex array */
  cp[0] = (GLfloat) data->pen.x * data->scale_x;
  cp[1] = (GLfloat) data->pen.y * data->scale_y;
  z = __glcComputePixelCoordinates(cp, data);
  if (!__glcArrayAppend(data->controlPoints, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }

  /* Compute the bounding box dimensions in the viewport coordinates */
  xMin = cp[5];
  xMax = cp[5];
  yMin = cp[6];
  yMax = cp[6];
  /* Keep track of the z coordinate to check if the glyph is outside the
   * frustrum or not.
   */
  zMin = z;
  zMax = z;

  /* Append the first vertex of the curve to the vertex array */
  rank = GLC_ARRAY_LENGTH(data->vertexArray);
  if (!__glcArrayAppend(data->vertexArray, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    return 1;
  }

  /* Build the array of the control points */
  for (i = 1; i < inOrder; i++) {
    cp[0] = (GLfloat) inControl[i-1]->x * data->scale_x;
    cp[1] = (GLfloat) inControl[i-1]->y * data->scale_y;
    z = __glcComputePixelCoordinates(cp, data);
    if (!__glcArrayAppend(data->controlPoints, cp)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      GLC_ARRAY_LENGTH(data->controlPoints) = 0;
      GLC_ARRAY_LENGTH(data->vertexArray) = rank;
      return 1;
    }
    /* Update the bounding box dimensions in the viewport coordinates */
    xMin = (cp[5] < xMin ? cp[5] : xMin);
    xMax = (cp[5] > xMax ? cp[5] : xMax);
    yMin = (cp[6] < yMin ? cp[6] : yMin);
    yMax = (cp[6] > yMax ? cp[6] : yMax);
    zMin = (z < zMin ? z : zMin);
    zMax = (z > zMax ? z : zMax);
  }

  /* Append the last vertex of the curve to the control points array */
  cp[0] = (GLfloat) inVecTo->x * data->scale_x;
  cp[1] = (GLfloat) inVecTo->y * data->scale_y;
  z = __glcComputePixelCoordinates(cp, data);
  if (!__glcArrayAppend(data->controlPoints, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(data->controlPoints) = 0;
    GLC_ARRAY_LENGTH(data->vertexArray) = rank;
    return 1;
  }
  /* Update the bounding box dimensions in the viewport coordinates */
  xMin = (cp[5] < xMin ? cp[5] : xMin);
  xMax = (cp[5] > xMax ? cp[5] : xMax);
  yMin = (cp[6] < yMin ? cp[6] : yMin);
  yMax = (cp[6] > yMax ? cp[6] : yMax);
  zMin = (z < zMin ? z : zMin);
  zMax = (z > zMax ? z : zMax);

  /* controlPoint[] must be computed there because data->controlPoints->data
   * may have been modified by a realloc() in __glcArrayInsert()
   */
  controlPoint = (GLfloat(*)[7])GLC_ARRAY_DATA(data->controlPoints);

  if (!data->displayListIsBuilding) {
    /* If the bounding box of the bezier curve lies outside the viewport then
     * the bezier curve is replaced by a piecewise linear curve that joins the
     * control points
     */
    if ((xMin > data->halfWidth) || (xMax < -data->halfWidth)
        || (yMin > data->halfHeight) || (yMax < -data->halfHeight)
	|| (zMin > 1.) || (zMax < -1.)) {
      for (i = 1; i < inOrder; i++) {
	if (!__glcArrayAppend(data->vertexArray, controlPoint[i])) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	  GLC_ARRAY_LENGTH(data->vertexArray) = rank;
	  return 1;
	}
      }
      GLC_ARRAY_LENGTH(data->controlPoints) = 0;
      return 0;
    }
  }

  /* Here the de Casteljau algorithm begins */
  for (iter = 0; (iter < GLC_MAX_ITER) && (arc != nArc); iter++) {
    GLint j = 0;
    GLfloat dmax = 0.;
    GLfloat ax = controlPoint[0][5];
    GLfloat ay = controlPoint[0][6];
    GLfloat abx = controlPoint[inOrder][5] - ax;
    GLfloat aby = controlPoint[inOrder][6] - ay;

    /* Normalize AB (the segment that joins the first and the last vertex of
     * the curve).
     */
    GLfloat normab = sqrt(abx * abx + aby * aby);
    abx /= normab;
    aby /= normab;

    /* For each control point, compute its chordal distance that is its
     * distance from the segment AB
     */
    for (i = 1; i < inOrder; i++) {
      GLfloat cpx = controlPoint[i][5] - ax;
      GLfloat cpy = controlPoint[i][6] - ay;
      GLfloat s = cpx * abx + cpy * aby;
      GLfloat projx = s * abx - cpx;
      GLfloat projy = s * aby - cpy;

      /* Compute the chordal distance */
      GLfloat distance = projx * projx + projy * projy;

      dmax = distance > dmax ? distance : dmax;
    }

    if (dmax < data->tolerance) {
      arc++; /* Process the next arc */
      controlPoint = ((GLfloat(*)[7])GLC_ARRAY_DATA(data->controlPoints))
	+ arc * inOrder;
      /* Update the place where new vertices will be inserted in the vertex
       * array
       */
      rank++;
    }
    else {
      /* Split an arc into two smaller arcs (this is the actual de Casteljau
       * algorithm)
       */
      GLfloat *cp1 = NULL;

      for (i = 0; i < inOrder; i++) {
	GLfloat *p1, *p2;

	cp1 = (GLfloat*)__glcArrayInsertCell(data->controlPoints,
					       arc*inOrder+i+1);
	if (!cp1) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  GLC_ARRAY_LENGTH(data->controlPoints) = 0;
	  return 1;
	}

	/* controlPoint[] must be updated there because
	 * data->controlPoints->data may have been modified by a realloc() in
	 * __glcArrayInsert()
	 */
	controlPoint = ((GLfloat(*)[7])GLC_ARRAY_DATA(data->controlPoints))
	  + arc * inOrder;

	p1 = controlPoint[i];
	p2 = controlPoint[i+1];

	cp1[0] = 0.5*(p1[0]+p2[0]);
	cp1[1] = 0.5*(p1[1]+p2[1]);
	cp1[2] = 0.5*(p1[2]+p2[2]);
	cp1[3] = 0.5*(p1[3]+p2[3]);
	cp1[4] = 0.5*(p1[4]+p2[4]);
	cp1[5] = cp1[2] / cp1[4];
	cp1[6] = cp1[3] / cp1[4];

	for (j = 0; j < inOrder - i - 1; j++) {
	  p1 = controlPoint[i+j+2];
	  p2 = controlPoint[i+j+3];
	  p1[0] = 0.5*(p1[0]+p2[0]);
	  p1[1] = 0.5*(p1[1]+p2[1]);
	  p1[2] = 0.5*(p1[2]+p2[2]);
	  p1[3] = 0.5*(p1[3]+p2[3]);
	  p1[4] = 0.5*(p1[4]+p2[4]);
	  p1[5] = p1[2] / p1[4];
	  p1[6] = p1[3] / p1[4];
	}
      }

      /* The point in cp1[] is a point located on the Bezier curve : it must be
       * added to the vertex array
       */
      if (!__glcArrayInsert(data->vertexArray, rank+1, cp1)) {
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
  __glcRendererData *data = (__glcRendererData *) inUserData;

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
  __glcRendererData *data = (__glcRendererData *) inUserData;
  GLfloat vertex[2];

  vertex[0] = (GLfloat) data->pen.x * data->scale_x;
  vertex[1] = (GLfloat) data->pen.y * data->scale_y;
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
#else
static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo,
			void* inUserData)
#endif
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  FT_Vector *control[1];
  int error = 0;

  control[0] = inVecControl;
  error = __glcdeCasteljau(inVecTo, control, inUserData, 2);
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
#else
static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2,
			FT_Vector *inVecTo, void* inUserData)
#endif
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  FT_Vector *control[2];
  int error = 0;

  control[0] = inVecControl1;
  control[1] = inVecControl2;
  error = __glcdeCasteljau(inVecTo, control, inUserData, 3);
  data->pen = *inVecTo;

  return error;
}



/* Callback function that is called by the GLU when it is tesselating a
 * polygon. This function is needed when a Bezier curve is replaced by a
 * piecewise linear curve (which can occur when a curve is outside the
 * viewport). In such a case since the curve is roughly described some
 * intersections can occur between two contours in certain positions.
 */
static CALLBACK void __glcCombineCallback(GLdouble coords[3], void* vertex_data[4],
				 GLfloat weight[4], void** outData,
				 void* inUserData)
{
  __glcRendererData *data = (__glcRendererData*)inUserData;
  GLfloat vertex[2];
  /* Evil hack for 32/64 bits compatibility */
  union {
    void* ptr;
    int i;
  } intInPtr;

  /* Compute the new vertex and append it to the vertex array */
  vertex[0] = (GLfloat)coords[0];
  vertex[1] = (GLfloat)coords[1];
  if (!__glcArrayAppend(data->vertexArray, vertex)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* Returns the index of the new vertex in the vertex array */
  intInPtr.i = GLC_ARRAY_LENGTH(data->vertexArray)-1;
  *outData = intInPtr.ptr;
}



/* Callback function that is called by the GLU when it is rendering the
 * tesselated polygon. This function is needed to convert the indices of the
 * vertex array into the coordinates of the vertex.
 */
static CALLBACK void __glcVertexCallback(void* vertex_data, void* inUserData)
{
  __glcRendererData *data = (__glcRendererData*)inUserData;
  GLfloat(*vertexArray)[2] = (GLfloat(*)[2])GLC_ARRAY_DATA(data->vertexArray);
  /* Evil hack for 32/64 bits compatibility */
  union {
    void* ptr;
    int i;
  } intInPtr;

  intInPtr.ptr = vertex_data;
  glVertex2fv(vertexArray[intInPtr.i]);
}



/* Callback function that is called by the GLU whenever an error occur during
 * the tesselation of the polygon.
 */
static CALLBACK void __glcCallbackError(GLenum inErrorCode)
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
void __glcRenderCharScalable(__glcFont* inFont, __glcContextState* inState,
			     GLCenum inRenderMode,
			     GLboolean inDisplayListIsBuilding,
			     GLfloat* inTransformMatrix, GLfloat scale_x,
			     GLfloat scale_y,
			     __glcGlyph* inGlyph)
{
  FT_Outline *outline = NULL;
  FT_Outline_Funcs outlineInterface;
  __glcRendererData rendererData;
  GLfloat identityMatrix[16] = {1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.,
				 0., 0., 0., 0., 1.};
  FT_Face face = inFont->faceDesc->face;

  /* Initialize the data for FreeType to parse the outline */
  outline = &face->glyph->outline;
  outlineInterface.shift = 0;
  outlineInterface.delta = 0;
  outlineInterface.move_to = __glcMoveTo;
  outlineInterface.line_to = __glcLineTo;
  outlineInterface.conic_to = __glcConicTo;
  outlineInterface.cubic_to = __glcCubicTo;

  /* grid_coordinate is given in 26.6 fixed point integer hence we
     divide the scale by 2^6 */
  rendererData.scale_x = 1./64./scale_x;
  rendererData.scale_y = 1./64./scale_y;

  rendererData.vertexArray = inState->vertexArray;
  rendererData.controlPoints = inState->controlPoints;
  rendererData.endContour = inState->endContour;

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
    rendererData.transformMatrix[0] *= rendererData.halfWidth;
    rendererData.transformMatrix[4] *= rendererData.halfWidth;
    rendererData.transformMatrix[12] *= rendererData.halfWidth;
    rendererData.transformMatrix[1] *= rendererData.halfHeight;
    rendererData.transformMatrix[5] *= rendererData.halfHeight;
    rendererData.transformMatrix[13] *= rendererData.halfHeight;
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
      * face->units_per_EM * rendererData.scale_x * rendererData.scale_y;
    rendererData.halfWidth = 0.5;
    rendererData.halfHeight = 0.5;
    rendererData.transformMatrix = identityMatrix;
  }

  /* Parse the outline of the glyph */
  if (FT_Outline_Decompose(outline, &outlineInterface, &rendererData)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inState->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inState->endContour) = 0;
    return;
  }

  if (!__glcArrayAppend(rendererData.endContour,
			&GLC_ARRAY_LENGTH(rendererData.vertexArray))) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inState->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inState->endContour) = 0;
    return;
  }

  /* Prepare the display list compilation if needed */
  if (inState->glObjects) {
    int index = (inRenderMode == GLC_LINE) ? 1 : 2;

    inGlyph->displayList[index] = glGenLists(1);
    if (!inGlyph->displayList[index]) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      GLC_ARRAY_LENGTH(inState->vertexArray) = 0;
      GLC_ARRAY_LENGTH(inState->endContour) = 0;
      return;
    }

    glNewList(inGlyph->displayList[index], GL_COMPILE_AND_EXECUTE);
  }

  if (inRenderMode == GLC_TRIANGLE) {
    /* Tesselate the polygon defined by the contour returned by
     * FT_Outline_Decompose().
     */
    GLUtesselator *tess = gluNewTess();
    int i = 0, j = 0;
    int* endContour = (int*)GLC_ARRAY_DATA(rendererData.endContour);
    GLfloat (*vertexArray)[2] =
      (GLfloat(*)[2])GLC_ARRAY_DATA(rendererData.vertexArray);
    GLdouble coords[3] = {0., 0., 0.};

    /* Initialize the GLU tesselator */
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);

#ifndef __WIN32__
	gluTessCallback(tess, GLU_TESS_ERROR, (void (*) ())__glcCallbackError);
    gluTessCallback(tess, GLU_TESS_BEGIN, (void (*) ())glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA,
		    (void (*) ())__glcVertexCallback);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA,
		    (void (*) ())__glcCombineCallback);
    gluTessCallback(tess, GLU_TESS_END, glEnd);
#else
	gluTessCallback(tess, GLU_TESS_ERROR,
			(CALLBACK void (*) ())__glcCallbackError);
    gluTessCallback(tess, GLU_TESS_BEGIN, (CALLBACK void (*) ())glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA,
		    (CALLBACK void (*) ())__glcVertexCallback);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA,
		    (CALLBACK void (*) ())__glcCombineCallback);
    gluTessCallback(tess, GLU_TESS_END, (CALLBACK void (*) ())glEnd);
#endif

    gluTessNormal(tess, 0., 0., 1.);

    /* Define the polygon geometry */
    gluTessBeginPolygon(tess, &rendererData);

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++) {
      /* Evil hack for 32/64 bits compatibility */
      union {
	void* ptr;
	int i;
      } intInPtr;

      gluTessBeginContour(tess);
      for (j = endContour[i]; j < endContour[i+1]; j++) {
	coords[0] = (GLdouble)vertexArray[j][0];
	coords[1] = (GLdouble)vertexArray[j][1];
	intInPtr.i = j;
	gluTessVertex(tess, coords, intInPtr.ptr);
      }
      gluTessEndContour(tess);
    }

    /* Close the polygon and run the tesselation */
    gluTessEndPolygon(tess);

    /* Free memory */
    gluDeleteTess(tess);
  }
  else {
    /* For GLC_LINE, there is no need to tesselate. The vertex are contained
     * in an array so we use the OpenGL function glDrawArrays().
     */
    int i = 0;
    int* endContour = (int*)GLC_ARRAY_DATA(rendererData.endContour);

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glNormal3f(0., 0., 1.);
    glVertexPointer(2, GL_FLOAT, 0, GLC_ARRAY_DATA(rendererData.vertexArray));

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++)
      glDrawArrays(GL_LINE_LOOP, endContour[i], endContour[i+1]-endContour[i]);

    glPopClientAttrib();
  }

  /* Take into account the advance of the glyph */
  glTranslatef(face->glyph->advance.x * rendererData.scale_x,
	       face->glyph->advance.y * rendererData.scale_y, 0.);

  if (inState->glObjects)
    glEndList();

  GLC_ARRAY_LENGTH(inState->vertexArray) = 0;
  GLC_ARRAY_LENGTH(inState->endContour) = 0;
}
