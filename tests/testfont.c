/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

extern void my_init(void);
extern void my_fini(void);

void testQueso(void)
{
    int ctx = 0;
    int font = 0;
    
    my_init();

    ctx = glcGenContext();
    glcContext(ctx);
    
    glcFontFace(glcNewFontFromFamily(1, "Utopia"), "Bold");
    font = glcNewFontFromFamily(2, "Courier");
    glcFont(font);
    glcFontFace(font, "Italic");
    
    printf("Face : %s\n", (char *)glcGetFontFace(font));
    printf("GenFontID : %d\n", glcGenFontID());
    printf("Face 1 : %s\n", (char *)glcGetFontListc(font, GLC_FACE_LIST, 1));
    
    printf("Font count : %d\n", glcGeti(GLC_FONT_COUNT));
    printf("Current font count : %d\n", glcGeti(GLC_CURRENT_FONT_COUNT));
    printf("Font #%d : %d\n", 1, glcGetListi(GLC_FONT_LIST, 1));
    printf("Current Font #%d : %d\n", 1, glcGetListi(GLC_CURRENT_FONT_LIST, 1));
    
    printf("Font Map #%d : %s\n", 65, (char *)glcGetFontMap(font, 65));
    printf("Font Map #%d : %s\n", 92, (char *)glcGetFontMap(font, 92));
    
    glcFontMap(font, 90, "LATIN CAPITAL LETTER A");
    
    printf("Font Map #%d : %s\n", 65, (char *)glcGetFontMap(font, 65));
    my_fini();
}
