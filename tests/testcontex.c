/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

extern void my_init(void);
extern void my_fini(void);

void testQueso(void)
{
    int ctx = 0;
    
    my_init();
    
    printf("Error : %d\n", glcGetError());
    glcDisable(GLC_AUTO_FONT);
    printf("Error : %d\n", glcGetError());
    printf("Error : %d\n", glcGetError());
    
    ctx = glcGenContext();
    glcContext(ctx);
    printf("Error : %d\n", glcGetError());
    
    printf("AUTO_FONT : %s\n", glcIsEnabled(GLC_AUTO_FONT) ? "True" : "False");
    printf("GL_OBJECTS : %s\n", glcIsEnabled(GLC_GL_OBJECTS) ? "True" : "False");
    printf("MIPMAP : %s\n", glcIsEnabled(GLC_MIPMAP) ? "True" : "False");
    glcDisable(GLC_AUTO_FONT);
    printf("Disable : GLC_AUTO_FONT\n");

    printf("AUTO_FONT : %s\n", glcIsEnabled(GLC_AUTO_FONT) ? "True" : "False");
    printf("GL_OBJECTS : %s\n", glcIsEnabled(GLC_GL_OBJECTS) ? "True" : "False");
    printf("MIPMAP : %s\n", glcIsEnabled(GLC_MIPMAP) ? "True" : "False");
    glcDisable(GLC_GL_OBJECTS);
    printf("Disable : GLC_GL_OBJECTS\n");
    
    printf("AUTO_FONT : %s\n", glcIsEnabled(GLC_AUTO_FONT) ? "True" : "False");
    printf("GL_OBJECTS : %s\n", glcIsEnabled(GLC_GL_OBJECTS) ? "True" : "False");
    printf("MIPMAP : %s\n", glcIsEnabled(GLC_MIPMAP) ? "True" : "False");

    printf("Error : %d\n", glcGetError());
    glcDisable(0);
    printf("Error : %d\n", glcGetError());

    my_fini();
}
