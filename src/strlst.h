/* $Id$ */
#ifndef __glc_strlst_h
#define __glc_strlst_h

#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"

#define GLC_STRING_CHUNK	256

class __glcStringList {
	GLCchar* list;		/* Pointer to the string */
	GLint count;		/* # list elements */
	GLuint maxLength;	/* maximal length of the string */
	GLint stringType;	/* string type : UCS1, UCS2 or UCS4 */
	
	GLint grow(GLint inSize);

    public:
	inline GLint getCount(void) { return count; }
	
	__glcStringList(GLint inStringType);
	~__glcStringList();
	GLint append(const GLCchar* inString);
	GLint prepend(const GLCchar* inString);
	GLint remove(const GLCchar* inString);
	GLint removeIndex(GLuint inIndex);
	GLCchar* find(const GLCchar* inString);
	GLCchar* findIndex(GLuint inIndex);
	GLint getIndex(const GLCchar* inString);
	GLCchar* extract(GLint inIndex, GLCchar* inBuffer, GLint inBufferSize);
};

GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex, const GLCchar* inSeparator);

#endif /* __glc_strlst_h */
