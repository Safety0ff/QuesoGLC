/* Scalable font renderer (triangle & line modes) */
#include <GL/glu.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "internal.h"
#include FT_OUTLINE_H

#define GLC_MAX_VERTEX	256
#define GLC_SCALE	6553.6

typedef struct {
    GLUtesselator *tess;	/* GLU tesselator */
    FT_Vector pen;		/* Current coordinates */
    GLfloat tolerance;		/* Chordal tolerance */
    GLfloat (*vertex)[2];	/* Vertices array */
    GLint numVertex;		/* Number of vertices in the vertices array */
    GLboolean firstVertex;	/* This flag == GL_TRUE if '__glcMoveTo is called for the first time */
}__glcRendererData;

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
    GLfloat *vertex = &data->vertex[data->numVertex][0];
    GLdouble coords[3] = {0., 0., 0.};
    
    data->pen = *inVecTo;
    coords[0] = vertex[0] = (GLfloat) inVecTo->x;
    coords[1] = vertex[1] = (GLfloat) inVecTo->y;
    gluTessVertex(data->tess, coords, vertex);
    data->numVertex++;
    
    return 0;
}

static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2, FT_Vector *inVecTo, void* inUserData)
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

static void __glcVertexCallbackFunc(void *inVertexData)
{
    GLfloat *vertex = (GLfloat *) inVertexData;

    glNormal3f(0., 0., 1.);
    glVertex2f(vertex[0] / GLC_SCALE, vertex[1] / GLC_SCALE);
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
    rendererData.vertex = (GLfloat (*)[2]) malloc(GLC_MAX_VERTEX * sizeof(GLfloat));
    if (!rendererData.vertex) {
	gluDeleteTess(tess);
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return;
    }
    
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, !inFill);

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
