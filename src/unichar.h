#ifndef __glc_unichar_h
#define __glc_unichar_h

#include "GL/glc.h"
#include <string.h>

typedef union {
  unsigned char *ucs1;
  unsigned short *ucs2;
  unsigned int *ucs4;
} uniChar;

class __glcUniChar {
 public:
  GLCchar *ptr;
  GLint type;

  __glcUniChar();
  __glcUniChar(const GLCchar* inChar, GLint inType = GLC_UCS1);
  ~__glcUniChar() {}

  int len(void);
  int lenBytes(void);
  GLCchar* copy(__glcUniChar* dest) 
    { return (GLCchar *)memcpy(dest->ptr, ptr, lenBytes()); }
  int compare(__glcUniChar* inString);
};

#endif
