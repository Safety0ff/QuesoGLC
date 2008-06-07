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
/* $Id: test10.c 712 2008-01-26 19:46:49Z bcoconni $ */

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
  GLfloat width1 = 0.f;
  GLfloat width2 = 0.f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glColor3f(1.f, 0.f, 0.f);
  glLoadIdentity();
  glScalef(100.f, 100.f, 1.f);
  glTranslatef(0.5f, 0.5f, 0.f);
  glcDisable(GLC_KERNING_QSO);
  glPushMatrix();
  glcRenderString("AVG");
  glPopMatrix();
  glcMeasureString(GL_FALSE, "AVG");
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  width1 = bbox[4] - bbox[1];

  glColor3f(1.f, 0.f, 0.f);
  glLoadIdentity();
  glScalef(100.f, 100.f, 1.f);
  glTranslatef(0.5f, 1.5f, 0.f);
  glcEnable(GLC_KERNING_QSO);
  glPushMatrix();
  glcRenderString("AVG");
  glPopMatrix();
  glcMeasureString(GL_FALSE, "AVG");
  glcGetStringMetric(GLC_BOUNDS, bbox);
  glColor3f(0.f, 1.f, 1.f);
  glBegin(GL_LINE_LOOP);
  for (i = 0; i < 4; i++)
    glVertex2fv(&bbox[2*i]);
  glEnd();
  width2 = bbox[4] - bbox[1];

  glFlush();

  printf("Width /wo kerning: %f\nWidth /w kerning : %f\n", width1, width2);
}

int main(int argc, char **argv)
{
  GLint ctx = 0;
  GLint myFont = 0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(640, 300);
  glutCreateWindow("Test12");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glEnable(GL_TEXTURE_2D);

  /* Set up and initialize GLC */
  ctx = glcGenContext();
  glcContext(ctx);
  glcAppendCatalog("/usr/lib/X11/fonts/Type1");
  glcDisable(GLC_GL_OBJECTS);

  /* Create a font "Palatino Bold" */
  myFont = glcGenFontID();
#ifdef __WIN32__
  glcNewFontFromFamily(myFont, "Times New Roman");
#else
  glcNewFontFromFamily(myFont, "DejaVu Serif");
#endif
  glcFontFace(myFont, "Book");
  glcFont(myFont);
  glcRenderStyle(GLC_TEXTURE);

  glutMainLoop();
  return 0;
}
