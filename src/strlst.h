/* $Id$ */
#ifndef __glc_strlst_h
#define __glc_strlst_h

#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "unichar.h"

#define GLC_STRING_CHUNK	256

class __glcStringList {
  __glcUniChar* string;	/* Pointer to the string */
  __glcStringList* next;
  GLuint count;

 public:
  inline GLint getCount(void) { return count; }

  __glcStringList(const GLCchar* inString = NULL, GLint inStringType = GLC_UCS1);
  ~__glcStringList();
  GLint append(const GLCchar* inString);
  GLint prepend(const GLCchar* inString);
  GLint remove(const GLCchar* inString);
  GLint removeIndex(GLuint inIndex);
  GLCchar* find(const GLCchar* inString);
  GLCchar* findIndex(GLuint inIndex);
  GLint getIndex(const GLCchar* inString);
};

GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex, const GLCchar* inSeparator);

#endif /* __glc_strlst_h */
