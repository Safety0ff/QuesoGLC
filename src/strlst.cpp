/* $Id$ */
/* Methods of __glcStringList objects 
 * IMPORTANT : they use ASCII strings instead of Unicode ones !!!
 */
#include "strlst.h"

__glcStringList::__glcStringList(const GLCchar* inString, GLint inStringType)
{
  __glcUniChar newString = __glcUniChar(inString, inStringType);

  string = new __glcUniChar(NULL, inStringType);
  if (inString) {
    string->ptr = (GLCchar *)malloc(newString.lenBytes());
    newString.copy(string);
  }
  next = NULL;
  count = 0;
}

__glcStringList::~__glcStringList()
{
  free(string->ptr);
  delete string;
}

GLint __glcStringList::append(const GLCchar* inString)
{
  __glcStringList *item = this;
  __glcStringList *current = this;

  if (count) {
    do {
      item->count++;
      current = item;
      item = item->next;
    } while (item);

    item = new __glcStringList(inString, string->type);
    if (!item)
      return -1;
    current->next = item;
    item->count++;
  }
  else {
    __glcUniChar s = __glcUniChar(inString, string->type);
    string->ptr = (GLCchar *)malloc(s.lenBytes());
    s.copy(string);
    count = 1;
  }

  return 0;
}

GLint __glcStringList::prepend(const GLCchar* inString)
{
  __glcStringList *item = NULL;
  __glcUniChar* temp = NULL;

  if (count) {
    item = new __glcStringList(inString, string->type);
    if (!item)
      return -1;
    temp = string;
    string = item->string;
    item->string = temp;
    item->next = next;
    next = item;
    item->count = count++;
  }
  else {
    __glcUniChar s = __glcUniChar(inString, string->type);
    string->ptr = (GLCchar *)malloc(s.lenBytes());
    s.copy(string);
    count = 1;
  }

  return 0;
}

GLint __glcStringList::remove(const GLCchar* inString)
{
  return removeIndex(getIndex(inString));
}

GLint __glcStringList::removeIndex(GLuint inIndex)
{
  __glcStringList *list = NULL;

  // String not found
  if (inIndex > count)
    return -1;

  if (!inIndex) {
    free(string->ptr);
    delete string;
    if (next) {
      string = next->string;
      next->string = NULL;
      count = next->count;
      list = next;
      next = next->next;
      delete list;
    }
    else {
      string = NULL;
      count = 0;
    }
  }
  else {
    GLuint index = 0;
    __glcStringList *current = this;
    list = this;

    while (index != inIndex) {
      list->count--;
      list = list->next;
      current = list;
      index++;
    }
    current->next = list->next;
    delete list;
  }

  return 0;
}

GLCchar* __glcStringList::find(const GLCchar* inString)
{
  return findIndex(getIndex(inString));
}

GLCchar* __glcStringList::findIndex(GLuint inIndex)
{
  __glcStringList *item = this;
  GLuint index = 0;

  if ((inIndex > count) || (inIndex < 0))
    return NULL;

  while(index != inIndex) {
    item = item->next;
    index++;
  };

  return item->string->ptr;
}

GLint __glcStringList::getIndex(const GLCchar* inString)
{
  __glcStringList *item = this;
  int index = 0;
  __glcUniChar s = __glcUniChar(inString, string->type);

  do {
    if (item->string->ptr) {
      if (!item->string->compare(&s))
	break;
    }
    item = item->next;
    index++;
  } while (item);

  if (!item)
    return -1;

  return index;
}

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
