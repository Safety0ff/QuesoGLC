/* $Id$ */
#include "internal.h"
#include <stdio.h>

void testQueso(void) {
    __glcStringList sl;
    __glcContextState state;
    char *string[] = { "string 1", "string 2", "string 3", "string 4", "foobar" };
    int i = 0;
    char buffer[256];
    
    state.stringType = GLC_UCS1;
    __glcStringListInit(&sl, &state);

    printf("Append:\n");
    for (i=0; i<3; i++) {
	__glcStringListAppend(&sl, string[i]);
	printf("%s<end> element : %d\n", (char *)sl.list, sl.count);
    }
    printf("Prepend:\n");
    __glcStringListPrepend(&sl, string[3]);
    printf("%s<end> element : %d\n", (char *)sl.list, sl.count);

    printf("FindIndex:\n");
    for (i=0; i<5; i++)
	printf("%d : %s<end>\n", i, (char *)__glcStringListFindIndex(&sl, i));
    i = 10;
    printf("%d : %s<end>\n", i, (char *)__glcStringListFindIndex(&sl, i));

    printf("Find:\n");
    for (i=0; i<5; i++)
	printf("%s : %s<end>\n", string[i], (char *)__glcStringListFind(&sl, string[i]));

    printf("Remove:\n");
    __glcStringListRemove(&sl, string[0]);
    printf("%s : %s<end> element : %d\n", string[0], (char *)sl.list, sl.count);
    __glcStringListRemove(&sl, string[2]);
    printf("%s : %s<end> element : %d\n", string[2], (char *)sl.list, sl.count);
    __glcStringListRemove(&sl, string[3]);
    printf("%s : %s<end> element : %d\n", string[3], (char *)sl.list, sl.count);
    __glcStringListRemove(&sl, string[4]);
    printf("%s : %s<end> element : %d\n", string[4], (char *)sl.list, sl.count);

    printf("\n");
    for (i=0; i<2; i++) {
	__glcStringListAppend(&sl, string[i]);
    }
    __glcStringListPrepend(&sl, string[3]);
    printf("%s<end> element : %d\n", (char *)sl.list, sl.count);

    printf("RemoveIndex:\n");
    __glcStringListRemoveIndex(&sl, 1);
    printf("%d : %s<end> element : %d\n", 1, (char *)sl.list, sl.count);
    __glcStringListRemoveIndex(&sl, 0);
    printf("%d : %s<end> element : %d\n", 0, (char *)sl.list, sl.count);
    __glcStringListRemoveIndex(&sl, 1);
    printf("%d : %s<end> element : %d\n", 1, (char *)sl.list, sl.count);
    __glcStringListRemoveIndex(&sl, 10);
    printf("%d : %s<end> element : %d\n", 10, (char *)sl.list, sl.count);
    __glcStringListRemoveIndex(&sl, 0);
    printf("%d : %s<end> element : %d\n", 0, (char *)sl.list, sl.count);
    __glcStringListRemoveIndex(&sl, 10);
    printf("%d : %s<end> element : %d\n", 10, (char *)sl.list, sl.count);

    printf("\n");
    for (i=0; i<2; i++) {
	__glcStringListAppend(&sl, string[i]);
    }
    __glcStringListPrepend(&sl, string[3]);
    printf("%s<end>\n", (char *)sl.list);

    printf("GetIndex:\n");
    for(i = 0; i < 5; i++)
	printf("%s index %d\n", string[i], __glcStringListGetIndex(&sl, string[i]));
    
    printf("Extract Element:\n");
    for(i = 0; i < 5; i++) {
	printf("String : %s elements : %d\n", (char *)sl.list, sl.count);
	printf("element #%d : %s\n", i, (char *)__glcStringListExtractElement(&sl, i, buffer, 256));
    }
}
