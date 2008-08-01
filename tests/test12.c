/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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
 * Regression test for bug #1987563 (reported by GenPFault) :
 *
 * glcEnable(GLC_KERNING_QSO) does not enable kerning.
 * Microsoft Word 2003 and the official Freetype tutorial kerning algorithm
 * both produced the correct kerning which is different from the kerning
 * obtained with QuesoGLC.
 */

#include "GL/glc.h"
#if defined __APPLE__ && defined __MACH__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <stdlib.h>
#include <stdio.h>

GLuint id = 0;


void reshape(int width, int height)
{
  glClearColor(0., 0., 0., 0.);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0., width, 0., height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
  switch(key) {
  case 27: /* Escape Key */
    exit(0);
  default:
    break;
  }
}

void display(void)
{
  int i = 0;
  GLfloat bbox[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
  GLfloat bbox2[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
  char string[20];

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Render GLC_BITMAP without kerning */
  glLoadIdentity();
  glColor3f(1.f, 0.f, 0.f);
  glRasterPos2f(50.f, 50.f);
  glcDisable(GLC_KERNING_QSO);
  glcRenderStyle(GLC_BITMAP);
  glcLoadIdentity();
  glcScale(100.f, 100.f);
  glcRenderString("AVG");
  glcMeasureString(GL_FALSE, "AVG");
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glTranslatef(50.f, 50.f, 0.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  /* Display the dimensions */
  snprintf(string, 20, "%f", bbox[2] - bbox[0]);
  glcEnable(GLC_HINTING_QSO);
  glcScale(0.15f, 0.15f);
  glcMeasureString(GL_FALSE, string);
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glColor3f(1.f, 1.f, 1.f);
  glBegin(GL_LINE);
  glVertex2fv(bbox);
  glVertex2f(bbox[0], bbox[1] - 40.f);
  glVertex2fv(&bbox[2]);
  glVertex2f(bbox[2], bbox[3] - 40.f);
  glVertex2f(bbox[0], bbox[1] - 30.f);
  glVertex2f(bbox[2], bbox[3] - 30.f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[0] + 5.f, bbox[1] - 25.f);
  glVertex2f(bbox[0], bbox[1] - 30.f);
  glVertex2f(bbox[0] + 5.f, bbox[1] - 35.f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[2] - 5.f, bbox[1] - 25.f);
  glVertex2f(bbox[2], bbox[1] - 30.f);
  glVertex2f(bbox[2] - 5.f, bbox[1] - 35.f);
  glEnd();
  glLoadIdentity();
  glRasterPos2f((bbox[2] - bbox[0] - (bbox2[2] - bbox2[0])) / 2.f + 50.f,
		bbox[1] + 23.f);
  glcRenderString(string);
  glcDisable(GLC_HINTING_QSO);

  /* Render GLC_TEXTURE without kerning */
  glLoadIdentity();
  glcRenderStyle(GLC_TEXTURE);
  glColor3f(1.f, 0.f, 0.f);
  glScalef(100.f, 100.f, 1.f);
  glTranslatef(3.f, 0.5f, 0.f);
  glPushMatrix();
  glcEnable(GLC_GL_OBJECTS);
  glcRenderString("AVG");
  glPopMatrix();
  glcMeasureString(GL_FALSE, "AVG");
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  /* Display the dimensions */
  snprintf(string, 20, "%f", (bbox[2] - bbox[0]) * 100.f);
  glcDisable(GLC_GL_OBJECTS);
  glcEnable(GLC_HINTING_QSO);
  glcMeasureString(GL_FALSE, string);
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glColor3f(1.f, 1.f, 1.f);
  glBegin(GL_LINE);
  glVertex2fv(bbox);
  glVertex2f(bbox[0], bbox[1] - 0.4f);
  glVertex2fv(&bbox[2]);
  glVertex2f(bbox[2], bbox[3] - 0.4f);
  glVertex2f(bbox[0], bbox[1] - 0.3f);
  glVertex2f(bbox[2], bbox[3] - 0.3f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[0] + 0.05f, bbox[1] - 0.25f);
  glVertex2f(bbox[0], bbox[1] - 0.3f);
  glVertex2f(bbox[0] + 0.05f, bbox[1] - 0.35f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[2] - 0.05f, bbox[1] - 0.25f);
  glVertex2f(bbox[2], bbox[1] - 0.3f);
  glVertex2f(bbox[2] - 0.05f, bbox[1] - 0.35f);
  glEnd();
  glTranslatef((bbox[2] - bbox[0] - (bbox2[2] - bbox2[0]) * 0.15f) / 2.f,
	       bbox[1] - 0.27f, 0.f);
  glScalef(0.15f, 0.15f, 1.f);
  glcRenderString(string);
  glcDisable(GLC_HINTING_QSO);

  /* Render GLC_BITMAP with kerning */
  glColor3f(1.f, 0.f, 0.f);
  glcEnable(GLC_KERNING_QSO);
  glcRenderStyle(GLC_BITMAP);
  glcLoadIdentity();
  glcScale(100.f, 100.f);
  glLoadIdentity();
  glRasterPos2f(50.f, 150.f);
  glcRenderString("AVG");
  glcMeasureString(GL_FALSE, "AVG");
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glTranslatef(50.f, 150.f, 0.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  /* Display the dimensions */
  snprintf(string, 20, "%f", bbox[4] - bbox[6]);
  glcEnable(GLC_HINTING_QSO);
  glcScale(0.15f, 0.15f);
  glcMeasureString(GL_FALSE, string);
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glColor3f(1.f, 1.f, 1.f);
  glBegin(GL_LINE);
  glVertex2fv(&bbox[4]);
  glVertex2f(bbox[4], bbox[5] + 40.f);
  glVertex2fv(&bbox[6]);
  glVertex2f(bbox[6], bbox[7] + 40.f);
  glVertex2f(bbox[4], bbox[5] + 30.f);
  glVertex2f(bbox[6], bbox[7] + 30.f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[4] - 5.f, bbox[5] + 25.f);
  glVertex2f(bbox[4], bbox[5] + 30.f);
  glVertex2f(bbox[4] - 5.f, bbox[5] + 35.f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[6] + 5.f, bbox[7] + 25.f);
  glVertex2f(bbox[6], bbox[7] + 30.f);
  glVertex2f(bbox[6] + 5.f, bbox[7] + 35.f);
  glEnd();
  glLoadIdentity();
  glRasterPos2f((bbox[4] - bbox[6] - (bbox2[4] - bbox2[6])) / 2.f + 50.f,
		bbox[7] + 183.f);
  glcRenderString(string);
  glcScale(2.f, 2.f);
  glcMeasureString(GL_FALSE, "GL_BITMAP");
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glRasterPos2f((bbox[2] - bbox[0] - (bbox2[2] - bbox2[0])) / 2.f + 50.f,
		300.f);
  glcRenderString("GL_BITMAP");
  glcDisable(GLC_HINTING_QSO);

  /* Render GLC_TEXTURE with kerning */
  glLoadIdentity();
  glcRenderStyle(GLC_TEXTURE);
  glColor3f(1.f, 0.f, 0.f);
  glScalef(100.f, 100.f, 1.f);
  glTranslatef(3.f, 1.5f, 0.f);
  glPushMatrix();
  glcEnable(GLC_GL_OBJECTS);
  glcRenderString("AVG");
  glPopMatrix();
  glcMeasureString(GL_TRUE, "AVG");
  glcGetStringCharMetric(1, GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 0.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  /* Display the dimensions */
  snprintf(string, 20, "%f", (bbox[4] - bbox[6]) * 100.f);
  glcDisable(GLC_GL_OBJECTS);
  glcEnable(GLC_HINTING_QSO);
  glcMeasureString(GL_FALSE, string);
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glColor3f(1.f, 1.f, 1.f);
  glBegin(GL_LINE);
  glVertex2fv(&bbox[4]);
  glVertex2f(bbox[4], bbox[5] + 0.4f);
  glVertex2fv(&bbox[6]);
  glVertex2f(bbox[6], bbox[7] + 0.4f);
  glVertex2f(bbox[4], bbox[5] + 0.3f);
  glVertex2f(bbox[6], bbox[7] + 0.3f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[6] + 0.05f, bbox[7] + 0.25f);
  glVertex2f(bbox[6], bbox[7] + 0.3f);
  glVertex2f(bbox[6] + 0.05f, bbox[7] + 0.35f);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox[4] - 0.05f, bbox[5] + 0.25f);
  glVertex2f(bbox[4], bbox[5] + 0.3f);
  glVertex2f(bbox[4] - 0.05f, bbox[5] + 0.35f);
  glEnd();
  glPushMatrix();
  glTranslatef((bbox[4] - bbox[6] - (bbox2[4] - bbox2[6]) * 0.15f) / 2.f,
	       bbox[5] + 0.33f, 0.f);
  glScalef(0.15f, 0.15f, 1.f);
  glcRenderString(string);
  glPopMatrix();
  glcMeasureString(GL_FALSE, "GL_TEXTURE");
  glcGetStringMetric(GLC_BOUNDS, bbox2);
  glTranslatef((bbox[2] - bbox[0] - (bbox2[2] - bbox2[0]) * 0.3f) / 2.f,
	       1.5f, 0.f);
  glScalef(0.3f, 0.3f, 1.f);
  glcRenderString("GL_TEXTURE");
  glcDisable(GLC_HINTING_QSO);

  glFlush();
}

int main(int argc, char **argv)
{
  GLint ctx = 0;
  GLint myFont = 0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(640, 400);
  glutCreateWindow("Test12");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glEnable(GL_TEXTURE_2D);

  /* Set up and initialize GLC */
  ctx = glcGenContext();
  glcContext(ctx);
  glcDisable(GLC_GL_OBJECTS);

  /* Create a font "Palatino Bold" */
  myFont = glcGenFontID();
#ifdef __WIN32__
  glcNewFontFromFamily(myFont, "Times New Roman");
  glcFontFace(myFont, "Regular");
#else
  glcNewFontFromFamily(myFont, "DejaVu Serif");
  glcFontFace(myFont, "Book");
#endif
  glcFont(myFont);
  glcRenderStyle(GLC_BITMAP);

  glutMainLoop();
  return 0;
}
