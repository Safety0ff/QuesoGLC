/* Scalable font renderer (triangle & line modes) */
#include <GL/glu.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_OUTLINE_H

#define GLC_MAX_VERTEX	1024
#define GLC_SCALE	6553.6

typedef struct __Node__ {
    GLfloat x;
    GLfloat y;
    GLfloat t;
    struct __Node__ *next;
} __glcNode;

typedef struct {
    GLUtesselator *tess;	/* GLU tesselator */
    FT_Vector pen;		/* Current coordinates */
    GLfloat tolerance;		/* Chordal tolerance */
    GLfloat (*vertex)[2];	/* Vertices array */
    GLint numVertex;		/* Number of vertices in the vertices array */
    GLboolean firstVertex;	/* This flag == GL_TRUE if '__glcMoveTo is called for the first time */
}__glcRendererData;

/* __glcdeCasteljau : renders bezier curves using "de Casteljau"'s algorithm
 * This function creates a piecewise linear curve which is close enough
 * to the real Bezier curve. The piecewise linear curve is built so that
 * the chordal distance is lower than a tolerance value.
 * The chordal distance is taken to be the perpendicular distance from the
 * parametric midpoint, (t = 0.5), to the chord. This may not always be correct,
 * but, in the small lengths which are being considered, this is good enough.
 * In order to mitigate any error, the chordal tolerance is taken to be slightly
 * smaller than the tolerance specified by the user. 
 * A second simplifying assumption is that when too large a tolerance is
 * encountered, the chord is split at the parametric midpoint, rather than
 * guessing the exact location of the best chord. This could lead to slightly
 * sub-optimal lines, but it provides a fast method for choosing the subdivision
 * point. This guess can be refined by lengthening the lines.
 */ 
static void __glcdeCasteljau(FT_Vector *inVecTo, FT_Vector **inControl, void *inUserData, GLint inOrder)
{
    GLboolean loop = GL_TRUE;
    __glcRendererData *data = (__glcRendererData *) inUserData;
    GLfloat x = (GLfloat) data->pen.x;
    GLfloat y = (GLfloat) data->pen.y;
    GLfloat tox = (GLfloat) inVecTo->x;
    GLfloat toy = (GLfloat) inVecTo->y;
    GLfloat cx[2], cy[2];
    GLfloat px[6], py[6];
    __glcNode nodeList[256];		/* FIXME : use a dynamic list */
    __glcNode *firstNode = &nodeList[0];
    __glcNode *currentNode = &nodeList[1];
    __glcNode *endList = currentNode;
    GLint i = 0;
    
    firstNode->x = x;
    firstNode->y = y;
    firstNode->t = 0.;
    firstNode->next = currentNode;
    
    currentNode->x = tox;
    currentNode->y = toy;
    currentNode->t = 1.;
    currentNode->next = NULL;
    
    for (i = 0; i < inOrder - 1; i++) {
	cx[i] = (GLfloat) inControl[i]->x;
	cy[i] = (GLfloat) inControl[i]->y;
    }
    
    while (loop) {
	GLfloat projx = 0., projy = 0.;
	__glcNode newNode;
	GLfloat s = 0., t = 0., distance = 0.;
	GLint j = 0;
	GLint n = 0, ind = 0;
	
	t = (firstNode->t + currentNode->t) / 2.;
	
	px[0] = (1 - t) * x + t * cx[0];
	py[0] = (1 - t) * x + t * cy[0];
	for (j = 0; j < inOrder - 2; j++) {
	    px[j + 1] = (1 - t) * cx[j] + t * cx[j + 1];
	    py[j + 1] = (1 - t) * cy[j] + t * cy[j + 1];
	}
	px[inOrder - 1] = (1 - t) * cx[inOrder - 2] + t * tox;
	py[inOrder - 1] = (1 - t) * cy[inOrder - 2] + t * toy;
	n = inOrder;
	
	for (i = 0; i < inOrder - 1 ; i++) {
	    for (j = 0; j < inOrder - i - 1; j++) {
		ind = n - inOrder - i + j;
		px[n + j] = (1 - t) * px[ind] + t * px[ind + 1];
		py[n + j] = (1 - t) * py[ind] + t * py[ind + 1];
	    }
	    n += inOrder - i - 1;
	}
	
	newNode.x = px[n - 1];
	newNode.y = py[n - 1];
	newNode.t = t;

	/* Compute the coordinate of the projection of 'new_node' onto the chord */
	s = ((newNode.x - firstNode->x) * (currentNode->x - firstNode->x)
	   + (newNode.y - firstNode->y) * (currentNode->y - firstNode->y))
	   / ((currentNode->x - firstNode->x) * (currentNode->x - firstNode->x)
	   +  (currentNode->y - firstNode->y) * (currentNode->y - firstNode->y));

	projx = firstNode->x + s * (currentNode->x - firstNode->x);
	projy = firstNode->y + s * (currentNode->y - firstNode->y);

	/* Compute the chordal distance */
	distance = (projx - newNode.x) * (projx - newNode.x)
		 + (projy - newNode.y) * (projy - newNode.y);
	
	/* If the chordal distance is greater than the tolerance then we
	   split the chord */
	if (distance > data->tolerance) {
	    /* Add the current node to the list (i.e. store the next
		chord ends) */
	    endList++;
	    endList->x = newNode.x;
	    endList->y = newNode.y;
	    endList->t = newNode.t;

	    /* The current chord ends are now 'first_node' and 'new_node' */
	    firstNode->next = endList;
	    endList->next = currentNode;
	    currentNode = endList;
	}
	else {
	    GLfloat *vertex = &data->vertex[data->numVertex][0];
	    GLdouble coords[3] = {0., 0., 0.};
    
	    coords[0] = vertex[0] = currentNode->x;
	    coords[1] = vertex[1] = currentNode->y;
	    gluTessVertex(data->tess, coords, vertex);
	    data->numVertex++;
	
	    if (currentNode->next) {
		/* Prepare to examine the next chord */
		firstNode = currentNode;
		currentNode = currentNode->next;
	    }
	    else
		/* No next chord : exit */
		loop = GL_FALSE;
	}
    }
}

static int __glcMoveTo(FT_Vector *inVecTo, void* inUserData)
{
    __glcRendererData *data = (__glcRendererData *) inUserData;
    
    if (!data->firstVertex) {
	gluTessEndContour(data->tess);
	gluTessBeginContour(data->tess);
    }
    
    data->pen = *inVecTo;
    data->firstVertex = GL_FALSE;
    return 0;
}

static int __glcLineTo(FT_Vector *inVecTo, void* inUserData)
{
    __glcRendererData *data = (__glcRendererData *) inUserData;
    GLfloat *vertex = &data->vertex[data->numVertex][0];
    GLdouble coords[3] = {0., 0., 0.};
    
    data->pen = *inVecTo;
    coords[0] = vertex[0] = (GLfloat) inVecTo->x;
    coords[1] = vertex[1] = (GLfloat) inVecTo->y;
    gluTessVertex(data->tess, coords, vertex);
    data->numVertex++;
    
    return 0;
}

static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo, void* inUserData)
{
    __glcRendererData *data = (__glcRendererData *) inUserData;
    FT_Vector *control[1];
    
    control[0] = inVecControl;
    
    __glcdeCasteljau(inVecTo, control, inUserData, 2);
    data->pen = *inVecTo;
    return 0;
}

static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2, FT_Vector *inVecTo, void* inUserData)
{
    __glcRendererData *data = (__glcRendererData *) inUserData;
    FT_Vector *control[2];
    
    control[0] = inVecControl1;
    control[1] = inVecControl2;
    
    __glcdeCasteljau(inVecTo, control, inUserData, 3);
    data->pen = *inVecTo;
    return 0;
}

static void __glcVertexCallbackFunc(void *inVertexData)
{
    GLfloat *vertex = (GLfloat *) inVertexData;

    glNormal3f(0., 0., 1.);
    glVertex2f(vertex[0] / GLC_SCALE, vertex[1] / GLC_SCALE);
}

static void __glcCallbackError(GLenum inErrorCode)
{
    __glcRaiseError(GLC_RESOURCE_ERROR);
}

void __glcRenderCharScalable(__glcFont* inFont, __glcContextState* inState, GLboolean inFill)
{
    FT_Outline *outline = NULL;
    FT_Outline_Funcs interface;
    
    __glcRendererData rendererData;
    GLUtesselator *tess = gluNewTess();

    outline = &inFont->face->glyph->outline;
    interface.shift = 0;
    interface.delta = 0;
    interface.move_to = __glcMoveTo;
    interface.line_to = __glcLineTo;
    interface.conic_to = __glcConicTo;
    interface.cubic_to = __glcCubicTo;
    
    rendererData.tess = tess;
    rendererData.numVertex = 0;
    rendererData.firstVertex = GL_TRUE;
    rendererData.tolerance = 0.81 * 4. * 4.;
    
    /* FIXME : may be we should use a bigger array ? */
    rendererData.vertex = (GLfloat (*)[2]) malloc(GLC_MAX_VERTEX * sizeof(GLfloat));
    if (!rendererData.vertex) {
	gluDeleteTess(tess);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, !inFill);

    gluTessCallback(tess, GLU_TESS_ERROR, __glcCallbackError);
    gluTessCallback(tess, GLU_TESS_BEGIN, glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX, __glcVertexCallbackFunc);
    gluTessCallback(tess, GLU_TESS_END, glEnd);
	
    gluTessNormal(tess, 0., 0., 1.);
    gluTessBeginPolygon(tess, &rendererData);
    gluTessBeginContour(tess);
    FT_Outline_Decompose(outline, &interface, &rendererData);
    gluTessEndContour(tess);
    gluTessEndPolygon(tess);
    
    glTranslatef(inFont->face->glyph->advance.x / GLC_SCALE, 0., 0.);
    
    gluDeleteTess(tess);
}
