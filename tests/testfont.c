/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

int main(void)
{
  int ctx = 0;
  int font = 0;
  int i = 0;
  int last = 0;

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

  for (i = 0; i < 5; i++)
    printf("Font #%d char #%d: %s\n", font, i,
	   (char*)glcGetFontListc(font, GLC_CHAR_LIST, i));

  printf("Font count : %d\n", glcGeti(GLC_FONT_COUNT));
  printf("Current font count : %d\n", glcGeti(GLC_CURRENT_FONT_COUNT));
  printf("Font #%d : %d\n", 1, glcGetListi(GLC_FONT_LIST, 1));
  printf("Current Font #%d : %d\n", 1, glcGetListi(GLC_CURRENT_FONT_LIST, 1));

  printf("Font Map #%d : %s\n", 65, (char *)glcGetFontMap(font, 65));
  printf("Font Map #%d : %s\n", 92, (char *)glcGetFontMap(font, 92));

  glcFontMap(font, 90, "LATIN CAPITAL LETTER A");

  printf("Font Map #%d : %s\n", 90, (char *)glcGetFontMap(font, 90));

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
