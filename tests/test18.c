/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2009, Bertrand Coconnier
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
 * Regression test for bug #2890444 (reported by Buginator)
 *
 * The bug is triggered by the following code sequence:
 * glcRenderStyle(GLC_TEXTURE); // Or any rendering style != GLC_BITMAP
 * glcEnable(GLC_GL_OBJECTS);
 * // Call measurement commands on a string which includes a character which is
 * // not mapped in the fonts of GLC_CURRENT_FONT_LIST
 * glcMeasureString(GL_FALSE, string);
 * 
 * glcMeasureString() then modifies the font map while it should not.
 */

#include "GL/glc.h"
#if defined __APPLE__ && defined __MACH__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <stdlib.h>
#include <stdio.h>

static GLint string[2] = {20027, 0};

int main(int argc, char **argv)
{
  GLint ctx = 0;
  GLint master = 0;
  GLint font = 0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(640, 200);
  glutCreateWindow("Test18");

  glEnable(GL_TEXTURE_2D);

  /* Set up and initialize GLC */
  ctx = glcGenContext();
  glcContext(ctx);
  glcRenderStyle(GLC_TEXTURE);
  glcStringType(GLC_UCS4);

  /* Look for a font which does not map the character in string[0] */
  for (master=0; master<glcGeti(GLC_MASTER_COUNT); master++) {
    if (!glcGetMasterMap(master, string[0]))
      break;
  }

  /* Select this font */
  font = glcGenFontID();
  glcNewFontFromMaster(font, master);
  glcFont(font);

  /* Display the name of the font and cross-check that it does not map string[0]
   */
  printf("Font #%d: %s\n", font, glcGetFontc(font, GLC_FAMILY));
  if (glcGetFontMap(font, string[0])) {
    printf("The char 0x%x is mapped in %s while it should not\n", string[0],
	   glcGetFontc(font, GLC_FAMILY));
    return -1;
  }

  /* Trigger the bug */
  glcMeasureString(GL_FALSE, string);

  /* Check if string[0] has been wrongly added to the font map */
  if (glcGetFontMap(font, string[0])) {
    printf("glcMeasureString() modified the font map\n");
    printf("Test failed\n");
    return -1;
  }

  printf("Test successful !\n");
  return 0;
}
