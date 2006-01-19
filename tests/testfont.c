/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2005, Bertrand Coconnier
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
#include "GL/glc.h"
#include <stdio.h>

int main(void)
{
  int ctx = 0;
  int font = 0;
  int i = 0;
  int last = 0;
  int count = 0;

  ctx = glcGenContext();
  glcContext(ctx);

  glcFontFace(glcNewFontFromFamily(1, "Utopia"), "Bold");
  font = glcNewFontFromFamily(2, "Courier");
  glcFont(font);
  glcFontFace(font, "Italic");

  printf("Face : %s\n", (char *)glcGetFontFace(font));
  printf("GenFontID : %d\n", glcGenFontID());
  printf("Font #%d family : %s\n", font,
	 (char *)glcGetFontc(font, GLC_FAMILY));
  for (i = 0; i < glcGetFonti(font, GLC_FACE_COUNT); i++)
    printf("Font #%d face #%d: %s\n", font, i,
	   (char *)glcGetFontListc(font, GLC_FACE_LIST, i));

  count = glcGetFonti(font, GLC_CHAR_COUNT);
  printf("Font #%d character count : %d\n", font, count);
  for (i = 0; i < count; i++)
    printf("Font #%d char #%d: %s\n", font, i,
	   (char*)glcGetFontListc(font, GLC_CHAR_LIST, i));

  printf("Font count : %d\n", glcGeti(GLC_FONT_COUNT));
  printf("Current font count : %d\n", glcGeti(GLC_CURRENT_FONT_COUNT));
  printf("Font #%d : %d\n", 1, glcGetListi(GLC_FONT_LIST, 1));
  printf("Current Font #%d : %d\n", 1, glcGetListi(GLC_CURRENT_FONT_LIST, 1));

  printf("Font Map #%d : %s\n", 65, (char *)glcGetFontMap(font, 65));
  printf("Font Map #%d : %s\n", 92, (char *)glcGetFontMap(font, 92));

  /* The characters maps are given in disorder in order to test the insertion
   * algorithm in glcFontMap()
   */
  glcFontMap(font, 87, "LATIN CAPITAL LETTER A");
  glcFontMap(font, 90, "LATIN CAPITAL LETTER D");
  glcFontMap(font, 89, "LATIN CAPITAL LETTER C");
  glcFontMap(font, 88, "LATIN CAPITAL LETTER B");

  /* Check that the characters are correctly mapped */
  for (i = 87; i < 91; i++)
    printf("Font Map #%d : %s\n", i, (char *)glcGetFontMap(font, i));

  /* The character code 88 is removed from the font map */
  glcFontMap(font, 88, NULL);
  printf("Character 88 removed from the font map\n");

  /* Check that the remaining characters in the font map are still correctly mapped */
  for (i = 87; i < 91; i++)
    printf("Font Map #%d : %s\n", i, (char *)glcGetFontMap(font, i));

  i = 1;
  last = glcGetMasteri(i, GLC_CHAR_COUNT);
  printf("Master #%d\n", i);
  printf("- Vendor : %s\n", (char *)glcGetMasterc(i, GLC_VENDOR));
  printf("- Format : %s\n", (char *)glcGetMasterc(i, GLC_MASTER_FORMAT));
  printf("- Face count : %d\n", glcGetMasteri(i, GLC_FACE_COUNT));
  printf("- Family : %s\n", (char *)glcGetMasterc(i, GLC_FAMILY));
  printf("- Min Mapped Code : %d\n", glcGetMasteri(i,GLC_MIN_MAPPED_CODE));
  printf("- Max Mapped Code : %d\n", glcGetMasteri(i,GLC_MAX_MAPPED_CODE));
  printf("- Char Count : %d\n", last);
  printf("- Last Character : %s\n", 
	 (char *)glcGetMasterListc(i, GLC_CHAR_LIST, last-1));

  glcRemoveCatalog(1);

  return 0;
}
