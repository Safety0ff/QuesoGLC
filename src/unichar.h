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
  GLCchar *ptr;
  GLint type;

  friend __glcStringList;

 public:
  __glcUniChar();
  __glcUniChar(const GLCchar* inChar, GLint inType = GLC_UCS1);
  ~__glcUniChar() {}

  int len(void);
  int lenBytes(void);
  GLCchar* dup(GLCchar* dest, size_t n)
    { return memmove(dest, ptr, lenBytes() > n ? n : lenBytes()); }
  int compare(__glcUniChar* inString);
  GLCchar* convert(GLCchar* dest, int inType,size_t n);
  int estimate(int inType);
  void destroy(void);
};

#endif
