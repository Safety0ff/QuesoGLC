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
    
    glcRenderStyle(GLC_BITMAP);
    glColor3f(1., 0., 0.);
    glRasterPos2f(30., 300.);
    glcScale(120., 120.);
    glcRotate(10.);
    glcRenderChar('O');
    glcRenderString("penGL");
    
    glcRenderStyle(GLC_TRIANGLE);
    glColor3f(1., 1., 0.);
    glTranslatef(30., 100., 0.);
    glScalef(120., 120., 0.);
    glcRenderChar('Q');
    glcRenderString("uesoGLC");
    
    my_fini();
}
