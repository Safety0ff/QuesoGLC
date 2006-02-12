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

#include "internal.h"
#include FT_OUTLINE_H
#include FT_LIST_H

typedef struct {
  FT_Vector pen;		/* Current coordinates */
  GLdouble tolerance;		/* Chordal tolerance */
  __glcArray* vertexArray;	/* Array of vertices */
  __glcArray* controlPoints;	/* Array of control points */
  __glcArray* endContour;	/* Array of contour limits */
  GLdouble scale_x;		/* Scale to convert grid point coordinates.. */
  GLdouble scale_y;		/* ..into pixel coordinates */
}__glcRendererData;

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
static int __glcdeCasteljau(FT_Vector *inVecTo, FT_Vector **inControl,
			    void *inUserData, GLint inOrder)
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  GLdouble(*controlPoint)[2] = NULL;
  GLdouble cp[3] = {0., 0., 0.};
  GLint i = 0, nArc = 1, arc = 0, rank = 0;

  /* Append the control points to the vertex array */
  cp[0] = (GLdouble) data->pen.x * data->scale_x;
  cp[1] = (GLdouble) data->pen.y * data->scale_y;
  if (!__glcArrayAppend(data->controlPoints, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  /* Append the first vertex of the curve to the vertex array */
  rank = GLC_ARRAY_LENGTH(data->vertexArray);
  if (!__glcArrayAppend(data->vertexArray, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  for (i = 1; i < inOrder; i++) {
    cp[0] = (GLdouble) inControl[i - 1]->x * data->scale_x;
    cp[1] = (GLdouble) inControl[i - 1]->y * data->scale_y;
    if (!__glcArrayAppend(data->controlPoints, cp)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return 1;
    }
  }
  cp[0] = (GLdouble) inVecTo->x * data->scale_x;
  cp[1] = (GLdouble) inVecTo->y * data->scale_y;
  if (!__glcArrayAppend(data->controlPoints, cp)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  /* controlPoint[] must be computed there because data->controlPoints->data
   * may have been modified by a realloc() in __glcArrayInsert()
   */
  controlPoint = (GLdouble(*)[2])GLC_ARRAY_DATA(data->controlPoints);

  while(1) {
    GLdouble projx = 0., projy = 0.;
    GLdouble s = 0., distance = 0., dmax = 0.;
    GLdouble ax = 0., ay = 0., abx = 0., aby = 0.;
    GLint j = 0;

    dmax = 0.;
    ax = controlPoint[0][0];
    ay = controlPoint[0][1];
    abx = controlPoint[inOrder][0] - ax;
    aby = controlPoint[inOrder][1] - ay;

    /* For each control point, compute its chordal distance that is its
     * distance from the line between the first and the last control points
     */
    for (i = 1; i < inOrder; i++) {
      cp[0] = controlPoint[i][0];
      cp[1] = controlPoint[i][1];

      /* Compute the chordal distance */
      s = ((cp[0] - ax) * abx + (cp[1] - ay) * aby)
	/ (abx * abx + aby * aby);
      projx = ax + s * abx;
      projy = ay + s * aby;

      distance = (projx - cp[0]) * (projx - cp[0])
	+ (projy - cp[1]) * (projy - cp[1]);
      dmax = distance > dmax ? distance : dmax;
    }

    if (dmax < data->tolerance) {
      arc++; /* Process the next arc */
      if (arc == nArc)
	break; /* Every arc has been processed : exit */
      controlPoint = ((GLdouble(*)[2])GLC_ARRAY_DATA(data->controlPoints))
	+ arc * inOrder;
      /* Update the place where new vertices will be inserted in the vertex
       * array
       */
      rank++;
    }
    else {
      /* Split an arc into two smaller arcs (this is the actual de Casteljau
       * algorithm
       */
      for (i = 0; i < inOrder; i++) {
	cp[0] = 0.5*(controlPoint[i][0]+controlPoint[i+1][0]);
	cp[1] = 0.5*(controlPoint[i][1]+controlPoint[i+1][1]);
	if (!__glcArrayInsert(data->controlPoints, arc*inOrder+i+1, cp)) {
	  __glcRaiseError(GLC_RESOURCE_ERROR);
	  return 1;
	}

	/* controlPoint[] must be updated there because
	 * data->controlPoints->data may have been modified by a realloc() in
	 * __glcArrayInsert()
	 */
	controlPoint = ((GLdouble(*)[2])GLC_ARRAY_DATA(data->controlPoints))
	  + arc * inOrder;

	for (j = 0; j < inOrder - i - 1; j++) {
	  controlPoint[i+j+2][0] = 0.5*(controlPoint[i+j+2][0]
					+controlPoint[i+j+3][0]);
	  controlPoint[i+j+2][1] = 0.5*(controlPoint[i+j+2][1]
					+controlPoint[i+j+3][1]);
	}
      }

      /* The point in cp[] is a point located on the Bezier curve : it must be
       * added to the vertex array
       */
      if (!__glcArrayInsert(data->vertexArray, rank+1, cp)) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
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

static int __glcMoveTo(FT_Vector *inVecTo, void* inUserData)
{
  __glcRendererData *data = (__glcRendererData *) inUserData;

  /* We don't need to store the point where the pen is since glyphs are defined
   * by closed loops (i.e. the first point and the last point are the same) and
   * the first point will be stored by the next call to lineto/conicto/cubicto.
   */

  if (!__glcArrayAppend(data->endContour, &GLC_ARRAY_LENGTH(data->vertexArray))) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  data->pen = *inVecTo;
  return 0;
}

static int __glcLineTo(FT_Vector *inVecTo, void* inUserData)
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  GLdouble vertex[3];

  vertex[0] = (GLdouble) data->pen.x * data->scale_x;
  vertex[1] = (GLdouble) data->pen.y * data->scale_y;
  vertex[2] = 0.;
  if (!__glcArrayAppend(data->vertexArray, vertex)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 1;
  }

  data->pen = *inVecTo;
  return 0;
}

static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo,
			void* inUserData)
{
  __glcRendererData *data = (__glcRendererData *) inUserData;
  FT_Vector *control[1];
  int error = 0;

  control[0] = inVecControl;
  error = __glcdeCasteljau(inVecTo, control, inUserData, 2);
  data->pen = *inVecTo;

  return error;
}

static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2,
			FT_Vector *inVecTo, void* inUserData)
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

/*static void __glcCombineCallback(GLdouble coords[3], void* vertex_data[4],
				 GLfloat weight[4], void** outData,
				 void* inUserData)
{
  __glcRendererData *data = (__glcRendererData*)inUserData;
  GLdouble(*vertexArray)[3] = (GLdouble(*)[3])data->vertexArray->data;

  if (!__glcArrayAppend(data->vertexArray, coords)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  *outData = vertexArray[data->vertexArray->length-1];
}*/

static void __glcCallbackError(GLenum inErrorCode)
{
  __glcRaiseError(GLC_RESOURCE_ERROR);
}

void __glcRenderCharScalable(__glcFont* inFont, __glcContextState* inState,
			     GLint inCode, GLboolean inFill, FT_Face inFace)
{
  FT_Outline *outline = NULL;
  FT_Outline_Funcs interface;
  FT_ListNode node = NULL;
  __glcDisplayListKey *dlKey = NULL;
  __glcRendererData rendererData;

  outline = &inFace->glyph->outline;
  interface.shift = 0;
  interface.delta = 0;
  interface.move_to = __glcMoveTo;
  interface.line_to = __glcLineTo;
  interface.conic_to = __glcConicTo;
  interface.cubic_to = __glcCubicTo;

  /* grid_coordinate is given in 26.6 fixed point integer hence we
     divide the scale by 2^6 */
  rendererData.scale_x = 1./64./GLC_POINT_SIZE;
  rendererData.scale_y = 1./64./GLC_POINT_SIZE;

  /* Compute the tolerance for the de Casteljau algorithm */
  rendererData.tolerance = 0.0005 * GLC_POINT_SIZE * inFace->units_per_EM
    * rendererData.scale_x * rendererData.scale_y;

  rendererData.vertexArray = inState->vertexArray;
  rendererData.controlPoints = inState->controlPoints;
  rendererData.endContour = inState->endContour;

  if (FT_Outline_Decompose(outline, &interface, &rendererData)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (!__glcArrayAppend(rendererData.endContour,
			&GLC_ARRAY_LENGTH(rendererData.vertexArray))) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  if (inState->glObjects) {
    dlKey = (__glcDisplayListKey *)__glcMalloc(sizeof(__glcDisplayListKey));
    if (!dlKey) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }

    dlKey->list = glGenLists(1);
    dlKey->faceDesc = inFont->faceDesc;
    dlKey->code = inCode;
    dlKey->renderMode = inFill ? 4 : 3;

    /* Get (or create) a new entry that contains the display list and store
     * the key in it
     */
    node = (FT_ListNode)dlKey;
    node->data = dlKey;
    FT_List_Add(&inFont->parent->displayList, node);

    glNewList(dlKey->list, GL_COMPILE);
  }

  if (inFill) {
    GLUtesselator *tess = gluNewTess();
    int i = 0, j = 0;
    int* endContour = (int*)GLC_ARRAY_DATA(rendererData.endContour);
    GLdouble (*vertexArray)[3] =
      (GLdouble(*)[3])GLC_ARRAY_DATA(rendererData.vertexArray);

    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);

    gluTessCallback(tess, GLU_TESS_ERROR, (void (*) ())__glcCallbackError);
    gluTessCallback(tess, GLU_TESS_BEGIN, (void (*) ())glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX, (void (*) ())glVertex3dv);
/*    gluTessCallback(tess, GLU_TESS_COMBINE_DATA,
		    (void (*) ())__glcCombineCallback);*/
    gluTessCallback(tess, GLU_TESS_END, glEnd);

    gluTessNormal(tess, 0., 0., 1.);

    gluTessBeginPolygon(tess, &rendererData);

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++) {
      gluTessBeginContour(tess);
      for (j = endContour[i]; j < endContour[i+1]; j++)
	gluTessVertex(tess, vertexArray[j], vertexArray[j]);
      gluTessEndContour(tess);
    }

    gluTessEndPolygon(tess);

    gluDeleteTess(tess);
  }
  else {
    int i = 0;
    int* endContour = (int*)GLC_ARRAY_DATA(rendererData.endContour);

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glNormal3f(0., 0., 1.);
    glVertexPointer(3, GL_DOUBLE, 0, GLC_ARRAY_DATA(rendererData.vertexArray));

    for (i = 0; i < GLC_ARRAY_LENGTH(rendererData.endContour)-1; i++)
      glDrawArrays(GL_LINE_LOOP, endContour[i], endContour[i+1]-endContour[i]);

    glPopClientAttrib();
  }

  glTranslatef(inFace->glyph->advance.x * rendererData.scale_x,
	       inFace->glyph->advance.y * rendererData.scale_y, 0.);

  if (inState->glObjects) {
    glEndList();
    glCallList(dlKey->list);
  }

  GLC_ARRAY_LENGTH(inState->vertexArray) = 0;
  GLC_ARRAY_LENGTH(inState->endContour) = 0;
}
