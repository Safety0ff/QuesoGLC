#include "unichar.h"

__glcUniChar::__glcUniChar()
{
  ptr = NULL;
  type = GLC_UCS1;
}

__glcUniChar::__glcUniChar(const GLCchar *inChar, GLint inType)
{
  ptr = (GLCchar *)inChar;
  type = inType;
}

int __glcUniChar::len(void)
{
  uniChar c;
  int length = 0;

  c.ucs1 = (unsigned char*)ptr;

  switch(type) {
  case GLC_UCS1:
    while (*(c.ucs1)) {
      c.ucs1++;
      length++;
    }
    return length;
  case GLC_UCS2:
    while (*(c.ucs2)) {
      c.ucs2++;
      length++;
    }
    return length;
  case GLC_UCS4:
    while (*(c.ucs4)) {
      c.ucs4++;
      length++;
    }
    return length;
  default:
    return 0;
  }

  return length;
}

int __glcUniChar::lenBytes(void)
{
  switch(type) {
  case GLC_UCS1:
    return len() + 1;
  case GLC_UCS2:
    return 2 * (len() + 1);
  case GLC_UCS4:
    return 4 * (len() + 1);
  default:
    return 0;
  }
}

int __glcUniChar::compare(__glcUniChar* inString)
{
  uniChar c1 , c2;

  c1.ucs1 = (unsigned char*) ptr;
  c2.ucs1 = (unsigned char*) inString->ptr;

  switch(type) {
  case GLC_UCS1:
    while(*(c1.ucs1)) {
      if (*(c1.ucs1) != *(c2.ucs1))
	break;
      c1.ucs1++;
      c2.ucs1++;
    }
    if (*(c1.ucs1) < *(c2.ucs1))
      return -1;
    if (*(c1.ucs1) == *(c2.ucs1))
      return 0;
    if (*(c1.ucs1) > *(c2.ucs1))
      return 1;
  case GLC_UCS2:
    while(*(c1.ucs2)) {
      if (*(c1.ucs2) != *(c2.ucs2))
	break;
      c1.ucs2++;
      c2.ucs2++;
    }
    if (*(c1.ucs2) < *(c2.ucs2))
      return -1;
    if (*(c1.ucs2) == *(c2.ucs2))
      return 0;
    if (*(c1.ucs2) > *(c2.ucs2))
      return 1;
  case GLC_UCS4:
    while(*(c1.ucs4)) {
      if (*(c1.ucs4) != *(c2.ucs4))
	break;
      c1.ucs4++;
      c2.ucs4++;
    }
    if (*(c1.ucs4) < *(c2.ucs4))
      return -1;
    if (*(c1.ucs4) == *(c2.ucs4))
      return 0;
    if (*(c1.ucs4) > *(c2.ucs4))
      return 1;
  default:
    return 0;
  }
}
