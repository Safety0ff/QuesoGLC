/* $Id$ */
/* Methods of __glcStringList objects 
 * IMPORTANT : they use ASCII strings instead of Unicode ones !!!
 */
#include "strlst.h"

/* FIXME : accesses to this buffer are not thread-safe !!! */
static char __glcTempBuffer[GLC_STRING_CHUNK];

 __glcStringList::__glcStringList(GLint inStringType)
{
    GLuint size = 0;
    
    switch(inStringType){
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
    
    list = (GLCchar *)malloc(size * GLC_STRING_CHUNK);
    if (!list) {
	return;
    }
    
    *((char *)(list)) = 0;
    count = 0;
    maxLength = GLC_STRING_CHUNK;
    stringType = inStringType;
    
    return;
}

__glcStringList::~__glcStringList()
{
    free(list);
    list = NULL;
    count = 0;
    maxLength = 0;
}

GLint __glcStringList::grow(GLint inSize)
{
    GLCchar *newList = NULL;
    GLuint size = 0;
    
    if ((strlen((const char*)list) + inSize) <= maxLength)
	return 0;
    
    switch(stringType){
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
    
    size *= inSize / GLC_STRING_CHUNK + 1;

    newList = realloc(list, maxLength + size * GLC_STRING_CHUNK);
    if (!newList)
	return 1;
	
    list = newList;
    maxLength += GLC_STRING_CHUNK;
    return 0;
}

static GLCchar* __glcRemoveSpaces(const GLCchar* inString)
{
    GLuint i = 0;

    strncpy(__glcTempBuffer, (const char*)inString, GLC_STRING_CHUNK);
    __glcTempBuffer[GLC_STRING_CHUNK - 1] = 0;
    
    for (i = 0; i < strlen(__glcTempBuffer); i++) {
	if (__glcTempBuffer[i] == ' ')
	    __glcTempBuffer[i]='_';
    }
    
    return __glcTempBuffer;
}

static GLCchar* __glcRestoreSpaces(const GLCchar* inString)
{
    GLuint i = 0;

    strncpy(__glcTempBuffer, (const char*)inString, GLC_STRING_CHUNK);
    __glcTempBuffer[GLC_STRING_CHUNK - 1] = 0;
    
    for (i = 0; i < strlen(__glcTempBuffer); i++) {
	if (__glcTempBuffer[i] == '_')
	    __glcTempBuffer[i]=' ';
    }
    
    return __glcTempBuffer;
}

GLint __glcStringList::append(const GLCchar* inString)
{
    char *s = (char *)inString;
    
    if (grow(strlen(s) + 2)) {
	return 1;
    }

    if (strlen((const char*)list))
	strncat((char*)list, " ", 1);
    strncat((char*)list, (const char*)__glcRemoveSpaces(inString), strlen((const char*)inString));
    count++;
    return 0;
}

GLint __glcStringList::prepend(const GLCchar* inString)
{
    char *s = (char *)inString;
    char *l = (char *)list;
    
    if (grow(strlen(s) + 2)) {
	return 1;
    }

    memmove(l + strlen(s) + 1, l, strlen(l) + 1);
    memcpy(l, __glcRemoveSpaces(inString), strlen(s));
    l[strlen(s)] = ' ';
    count++;
    return 0;
}

GLint __glcStringList::remove(const GLCchar* inString)
{
    char *s = (char*)find(__glcRemoveSpaces(inString));
    char *end = NULL;
    
    if (!s)
	return 0;

    end = s;
    while ((*end != ' ') && (*end))
	end++;

    if (s != (char *)list)
	--s;
    else {
	if (*end == ' ')
	    end++;
    }
    
    if (!(*end))
	*s = 0;
    else
	memmove(s, end, strlen(end)+1);

    count--;
    return 0;
}

GLint __glcStringList::removeIndex(GLuint inIndex)
{
    GLCchar *string = extract(inIndex, __glcTempBuffer, GLC_STRING_CHUNK);
    
    if (string)
	return remove(string);
    else
	return -1;
}

GLCchar* __glcStringList::find(const GLCchar* inString)
{
    if (strlen((const char*)inString))
	return (GLCchar *)strstr((const char *)list, (const char *)__glcRemoveSpaces(inString));
    else
	return NULL;
}

GLCchar* __glcFindIndexList(const GLCchar* inString, GLuint inIndex, const GLCchar* inSeparator)
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

GLCchar* __glcStringList::findIndex(GLuint inIndex)
{
    return __glcFindIndexList(list, inIndex, " ");
}

GLint __glcStringList::getIndex(const GLCchar* inString)
{
    char *s = (char *)list;
    char *end = NULL;
    GLuint occurence = 0;
    
    end = (char *)find(inString);
    if (!end)
	return -1;

    while (s != end) {
	if (*s == ' ')
	    occurence++;
	s++;
    }
    
    return occurence;
}

GLCchar* __glcStringList::extract(GLint inIndex, GLCchar* inBuffer, GLint inBufferSize)
{
    char *s = (char *)findIndex(inIndex);
    char *end = NULL;
    
    if (!(*s))
	return GLC_NONE;

    if (inIndex >= count)
	return GLC_NONE;
    
    if (inIndex + 1 == count) {
	strncpy((char *)inBuffer, (const char*)__glcRestoreSpaces(s), GLC_STRING_CHUNK);
	((char *)inBuffer)[inBufferSize - 1] = 0;
    }
    else {
	char temp = 0;
	
	end = (char *)findIndex(inIndex + 1);
	temp = *(--end);
	*end = 0;
	strncpy((char *)inBuffer, (const char*)__glcRestoreSpaces(s), GLC_STRING_CHUNK);
	((char *)inBuffer)[inBufferSize - 1] = 0;
	*end = temp;
    }
	
    return inBuffer;
}
