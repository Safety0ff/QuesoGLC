/* $Id$ */
/* Helper functions to manipulate __glcStringList objects 
 * IMPORTANT : they use ASCII strings instead of Unicode ones !!!
 */
#include "internal.h"
#include <string.h>
#include <stdlib.h>

static char __glcTempBuffer[GLC_STRING_CHUNK];

GLint __glcStringListInit(__glcStringList *inStringList, __glcContextState *inState)
{
    int size = 0;
    
    switch(inState->stringType){
	case GLC_UCS1:
	    size = sizeof(GLubyte);
	    break;
	case GLC_UCS2:
	    size = sizeof(GLushort);
	    break;
	case GLC_UCS4:
	    size = sizeof(GLint);
	    break;
    }
    
    inStringList->list = (GLCchar *)malloc(size * GLC_STRING_CHUNK);
    if (!inStringList->list) {
	return 1;
    }
    
    *((char *)(inStringList->list)) = 0;
    inStringList->count = 0;
    inStringList->maxLength = GLC_STRING_CHUNK;
    inStringList->stringType = inState->stringType;
    
    return 0;
}

void __glcStringListDelete(__glcStringList *inStringList)
{
    free(inStringList->list);
    inStringList->list = NULL;
    inStringList->count = 0;
    inStringList->maxLength = 0;
}

static GLint __glcStringListGrow(__glcStringList *inStringList, GLint inSize)
{
    GLCchar *newList = NULL;
    int size = 0;
    
    if ((strlen(inStringList->list) + inSize) <= inStringList->maxLength)
	return 0;
    
    switch(inStringList->stringType){
	case GLC_UCS1:
	    size = sizeof(char);
	    break;
	case GLC_UCS2:
	    size = sizeof(short);
	    break;
	case GLC_UCS4:
	    size = sizeof(long);
	    break;
    }
    
    size *= inSize / GLC_STRING_CHUNK + 1;

    newList = realloc(inStringList->list, inStringList->maxLength + size * GLC_STRING_CHUNK);
    if (!newList)
	return 1;
	
    inStringList->list = newList;
    inStringList->maxLength += GLC_STRING_CHUNK;
    return 0;
}

static GLCchar* __glcRemoveSpaces(const GLCchar* inString)
{
    int i = 0;

    strncpy(__glcTempBuffer, inString, GLC_STRING_CHUNK);
    __glcTempBuffer[GLC_STRING_CHUNK - 1] = 0;
    
    for (i = 0; i < strlen(__glcTempBuffer); i++) {
	if (__glcTempBuffer[i] == ' ')
	    __glcTempBuffer[i]='_';
    }
    
    return __glcTempBuffer;
}

static GLCchar* __glcRestoreSpaces(const GLCchar* inString)
{
    int i = 0;

    strncpy(__glcTempBuffer, inString, GLC_STRING_CHUNK);
    __glcTempBuffer[GLC_STRING_CHUNK - 1] = 0;
    
    for (i = 0; i < strlen(__glcTempBuffer); i++) {
	if (__glcTempBuffer[i] == '_')
	    __glcTempBuffer[i]=' ';
    }
    
    return __glcTempBuffer;
}

GLint __glcStringListAppend(__glcStringList *inStringList, const GLCchar* inString)
{
    char *s = (char *)inString;
    
    if (__glcStringListGrow(inStringList, strlen(s) + 2)) {
	return 1;
    }

    if (strlen(inStringList->list))
	strncat(inStringList->list, " ", 1);
    strncat(inStringList->list, __glcRemoveSpaces(inString), strlen(inString));
    inStringList->count++;
    return 0;
}

GLint __glcStringListPrepend(__glcStringList *inStringList, const GLCchar* inString)
{
    char *s = (char *)inString;
    char *list = (char *)inStringList->list;
    
    if (__glcStringListGrow(inStringList, strlen(s) + 2)) {
	return 1;
    }

    memmove(list + strlen(s) + 1, list, strlen(list) + 1);
    memcpy(list, __glcRemoveSpaces(inString), strlen(s));
    list[strlen(s)] = ' ';
    inStringList->count++;
    return 0;
}

GLint __glcStringListRemove(__glcStringList *inStringList, const GLCchar* inString)
{
    char *s = __glcStringListFind(inStringList, __glcRemoveSpaces(inString));
    char *end = NULL;
    
    if (!s)
	return 0;

    end = s;
    while ((*end != ' ') && (*end))
	end++;

    if (s != (char *)inStringList->list)
	--s;
    else {
	if (*end == ' ')
	    end++;
    }
    
    if (!(*end))
	*s = 0;
    else
	memmove(s, end, strlen(end)+1);

    inStringList->count--;
    return 0;
}

GLint __glcStringListRemoveIndex(__glcStringList *inStringList, GLint inIndex)
{
    GLCchar *string = __glcStringListExtractElement(inStringList, inIndex, __glcTempBuffer, GLC_STRING_CHUNK);
    
    if (string)
	return __glcStringListRemove(inStringList, string);
    else
	return -1;
}

GLCchar* __glcStringListFind(__glcStringList *inStringList, const GLCchar* inString)
{
    if (strlen(inString))
	return (GLCchar *)strstr((const char *)inStringList->list, (const char *)__glcRemoveSpaces(inString));
    else
	return NULL;
}

GLCchar* __glcFindIndexList(const GLCchar* inString, GLint inIndex, const GLCchar* inSeparator)
{
    int i = 0;
    int occurence = 0;
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

GLCchar* __glcStringListFindIndex(__glcStringList *inStringList, GLint inIndex)
{
    return __glcFindIndexList(inStringList->list, inIndex, " ");
}

GLint __glcStringListGetIndex(__glcStringList *inStringList, const GLCchar* inString)
{
    char *s = (char *)inStringList->list;
    char *end = NULL;
    int occurence = 0;
    
    end = (char *)__glcStringListFind(inStringList, inString);
    if (!end)
	return -1;

    while (s != end) {
	if (*s == ' ')
	    occurence++;
	s++;
    }
    
    return occurence;
}

GLCchar* __glcStringListExtractElement(__glcStringList *inStringList, GLint inIndex, GLCchar* inBuffer, GLint inBufferSize)
{
    char *s = (char *)__glcStringListFindIndex(inStringList, inIndex);
    char *end = NULL;
    
    if (!(*s))
	return GLC_NONE;

    if (inIndex >= inStringList->count)
	return GLC_NONE;
    
    if (inIndex + 1 == inStringList->count) {
	strncpy(inBuffer, __glcRestoreSpaces(s), GLC_STRING_CHUNK);
	((char *)inBuffer)[inBufferSize - 1] = 0;
    }
    else {
	char temp = 0;
	
	end = (char *)__glcStringListFindIndex(inStringList, inIndex + 1);
	temp = *(--end);
	*end = 0;
	strncpy(inBuffer, __glcRestoreSpaces(s), GLC_STRING_CHUNK);
	((char *)inBuffer)[inBufferSize - 1] = 0;
	*end = temp;
    }
	
    return inBuffer;
}
