/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

extern void my_init(void);
extern void my_fini(void);

void testQueso(void)
{
    int ctx = 0;
    int numMasters = 0;
    int numFaces = 0;
    int i = 0;
    int j = 0;
    
    my_init();
    
    ctx = glcGenContext();
    glcContext(ctx);
    
    glcAppendCatalog("/usr/lib/X11/fonts/Type1");
    
    printf("Catalogs : %d\nList : %s\n", glcGeti(GLC_CATALOG_COUNT), (const char *)glcGetListc(GLC_CATALOG_LIST, 0));
    
    numMasters = glcGeti(GLC_MASTER_COUNT);
    printf("Masters : %d\n", numMasters);
    
    for (i = 0; i < numMasters; i++) {
	numFaces = glcGetMasteri(i, GLC_FACE_COUNT);
	printf("Master #%d\n", i);
	printf("- Face count : %d\n", numFaces);
	printf("- Family : %s\n", (char *)glcGetMasterc(i, GLC_FAMILY));
	for (j = 1; j <= numFaces; j++)
	    printf("- Face #%d : %s\n", j, (char *)glcGetMasterListc(i, GLC_FACE_LIST, j));
    }
    
    my_fini();
}
