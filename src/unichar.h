#ifndef __glc_unichar_h
#define __glc_unichar_h

#include "GL/glc.h"
#include <string.h>

typedef union {
  unsigned char *ucs1;
  unsigned short *ucs2;
  unsigned int *ucs4;
} uniChar;

class __glcStringList;

class __glcUniChar {
 public:
  GLCchar *ptr;
  GLint type;

  friend __glcStringList;

  __glcUniChar();
  __glcUniChar(const GLCchar* inChar, GLint inType = GLC_UCS1);
  ~__glcUniChar() {}

  size_t len(void);
  size_t lenBytes(void);
  GLCchar* dup(GLCchar* dest, size_t n)
    { return memmove(dest, ptr, lenBytes() > n ? n : lenBytes()); }
  int compare(__glcUniChar* inString);
  GLCchar* convert(GLCchar* dest, int inType,size_t n);
  int estimate(int inType);
  void destroy(void);
  GLuint index(int inPos);
};

#endif
