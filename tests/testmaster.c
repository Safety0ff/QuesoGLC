/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

int main(void)
{
  int ctx = 0;
  int numMasters = 0;
  int numFaces = 0;
  int numCatalogs = 0;
  int i = 0;
  int j = 0;

  ctx = glcGenContext();
  glcContext(ctx);

  glcAppendCatalog("../fonts/Type1");

  numCatalogs = glcGeti(GLC_CATALOG_COUNT);
  printf("Catalogs : %d\n", numCatalogs);
  
  for (i = 0; i < numCatalogs; i++)
    printf("Catalog #%d : %s\n", i, (const char *)glcGetListc(GLC_CATALOG_LIST, i));

  numMasters = glcGeti(GLC_MASTER_COUNT);
  printf("Masters : %d\n", numMasters);

  for (i = 0; i < numMasters; i++) {
    numFaces = glcGetMasteri(i, GLC_FACE_COUNT);
    printf("Master #%d\n", i);
    printf("- Vendor : %s\n", (char *)glcGetMasterc(i, GLC_VENDOR));
    printf("- Format : %s\n", (char *)glcGetMasterc(i, GLC_MASTER_FORMAT));
    printf("- Face count : %d\n", numFaces);
    printf("- Family : %s\n", (char *)glcGetMasterc(i, GLC_FAMILY));
    for (j = 0; j < numFaces; j++)
      printf("- Face #%d : %s\n", j, 
	     (char *)glcGetMasterListc(i, GLC_FACE_LIST, j));
    printf("- Master #%d maps 0x%X to %s\n", i, 65 + i,
	   (char *)glcGetMasterMap(i, 65 + i));
  }

  return 0;
}
