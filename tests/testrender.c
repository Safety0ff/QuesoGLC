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
    
    glEnable(GL_TEXTURE_2D);
    
    ctx = glcGenContext();
    glcContext(ctx);
    
    glcAppendCatalog("/usr/lib/X11/fonts/Type1");
    
    font = glcNewFontFromFamily(glcGenFontID(), "Courier");
    glcFont(font);
    glcFontFace(font, "Italic");
    
    glcFontMap(font, 0x57, "LATIN SMALL LETTER W");
    
    glcRenderStyle(GLC_LINE);
    glColor3f(0., 1., 0.);
    glTranslatef(100., 50., 0.);
    glRotatef(45., 0., 0., 1.);
    glScalef(120., 120., 0.);
    glcRenderChar('L');
    glcRenderString("inux");
    glLoadIdentity();
    
    glcRenderStyle(GLC_TEXTURE);
    glColor3f(0., 0., 1.);
    glTranslatef(30., 350., 0.);
    glScalef(100., 100., 0.);
    glcRenderChar('X');
    glcRenderString("-windows");
    glLoadIdentity();
    
    glcRenderStyle(GLC_BITMAP);
    glColor3f(1., 0., 0.);
    glRasterPos2f(30., 200.);
    glcScale(120., 120.);
    glcRotate(10.);
    glcRenderChar('O');
    glcRenderString("penGL");
    
    glcRenderStyle(GLC_TRIANGLE);
    glColor3f(1., 1., 0.);
    glTranslatef(30., 50., 0.);
    glScalef(120., 120., 0.);
    glcRenderChar('Q');
    glcRenderString("uesoGLC");
    
    printf("Textures : %d\n", glcGeti(GLC_TEXTURE_OBJECT_COUNT));
    
    my_fini();
}
