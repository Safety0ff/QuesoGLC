/* $Id$ */
#ifndef __glc_strlst_h
#define __glc_strlst_h

#include <string.h>
#include <stdlib.h>

#include "GL/glc.h"
#include "unichar.h"

#define GLC_STRING_CHUNK	256

class __glcStringList {
 public:
  __glcUniChar* string;	/* Pointer to the string */
  __glcStringList* next;
  GLuint count;

  inline GLint getCount(void) { return count; }

  __glcStringList(__glcUniChar* inString);
  ~__glcStringList();
  GLint append(__glcUniChar* inString);
  GLint prepend(__glcUniChar* inString);
  GLint remove(__glcUniChar* inString);
  GLint removeIndex(GLuint inIndex);
  __glcUniChar* find(__glcUniChar* inString);
  __glcUniChar* findIndex(GLuint inIndex);
  GLint getIndex(__glcUniChar* inString);
  GLint convert(int inType);
};

GLCchar* __glcFindIndexList(const GLCchar* inList, GLuint inIndex, const GLCchar* inSeparator);

#endif /* __glc_strlst_h */
