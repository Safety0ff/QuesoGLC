#include <stdlib.h>
#include "internal.h"
#include FT_LIST_H

/* QuesoGLC own allocation and memory management routines */

void* __glcMalloc(size_t size)
{
  return malloc(size);
}

void __glcFree(void *ptr)
{
  free(ptr);
}

void* __glcRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}

/* FT lists destructor */
void __glcListDestructor(FT_Memory inMemory, void *inData, void *inUser)
{
  __glcFree(inData);
}

/* Find a token in a list of tokens separated by 'separator' */
GLCchar* __glcFindIndexList(const GLCchar* inString, GLuint inIndex,
			    const GLCchar* inSeparator)
{
    GLuint i = 0;
    GLuint occurence = 0;
    char *s = (char *)inString;
    char *sep = (char *)inSeparator;
    
    if (!inIndex)
	return (GLCchar *)inString;
    
    /* TODO use Unicode instead of ASCII */
    for (i=0; i<=strlen(s); i++) {
	if (s[i] == *sep)
	    occurence++;
	if (occurence == inIndex) {
	    i++;
	    break;
	}
    }
    
    
    return (GLCchar *)&s[i];
}
