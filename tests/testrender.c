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
    
    glcAppendCatalog("/usr/lib/X11/fonts/Type1");
    
    glcFontFace(glcNewFontFromFamily(1, "Utopia"), "Bold");
    font = glcNewFontFromFamily(2, "Courier");
    glcFont(font);
    glcFontFace(font, "Italic");
    
    glcRenderStyle(GLC_TRIANGLE);
    glTranslatef(100., 100., 0.);
    glScalef(10., 10., 0.);
    glColor3f(1., 1., 0.);
    glcRenderChar('Q');
    glcRenderString("uesoGLC");
    
    my_fini();
}
