/* $Id$ */
/* Methods of __glcStringList objects 
 */
#include "strlst.h"

__glcStringList::__glcStringList(__glcUniChar* inString)
{
  GLCchar *room = NULL;

  if (inString) {
    room = (GLCchar *)malloc(inString->lenBytes());
    inString->dup(room, inString->lenBytes());
    string = new __glcUniChar(room, inString->type);
    count = 1;
  }
  else {
    string = NULL;
    count = 0;
  }
  next = NULL;
}

__glcStringList::~__glcStringList()
{
  if (string) {
    free(string->ptr);
    delete string;
  }
}

GLint __glcStringList::append(__glcUniChar* inString)
{
  __glcStringList *item = this;
  __glcStringList *current = this;
  GLCchar *room = NULL;

  if (!inString)
    return -1;

  if (count) {
    __glcUniChar s;

    if (inString->type > string->type)
      if (convert(inString->type))
	return -1;

    do {
      item->count++;
      current = item;
      item = item->next;
    } while (item);

    room = (GLCchar *)malloc(inString->lenBytes());
    inString->convert(room, current->string->type, inString->lenBytes());
    s = __glcUniChar(room, current->string->type);
    item = new __glcStringList(&s);

    free(room);
    if (!item)
      return -1;
    current->next = item;
  }
  else {
    room = (GLCchar *)malloc(inString->lenBytes());
    inString->dup(room, inString->lenBytes());
    string = new __glcUniChar(room, inString->type);
    count = 1;
  }

  return 0;
}

GLint __glcStringList::prepend(__glcUniChar* inString)
{
  __glcStringList *item = NULL;
  __glcUniChar* temp = NULL;
  GLCchar *room = NULL;

  if (count) {
    if (inString->type > string->type)
      if (convert(inString->type))
	return -1;

    room = (GLCchar *)malloc(inString->lenBytes());
    inString->convert(room, string->type, inString->lenBytes());
    temp = new __glcUniChar(room, string->type);
    item = new __glcStringList(temp);

    free(room);
    delete temp;
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
    room = (GLCchar *)malloc(inString->lenBytes());
    inString->dup(room, inString->lenBytes());
    string = new __glcUniChar(room, inString->type);
    count = 1;
  }

  return 0;
}

GLint __glcStringList::remove(__glcUniChar* inString)
{
  return removeIndex(getIndex(inString));
}

GLint __glcStringList::removeIndex(GLuint inIndex)
{
  __glcStringList *list = NULL;

  // String not found
  if ((inIndex >= count) || (inIndex < 0))
    return -1;

  if (!inIndex) {
    free(string->ptr);
    delete string;
    
    if (next) {
      list = next;
      string = new __glcUniChar(list->string->ptr, list->string->type);
      list->string->ptr = NULL;
      count = list->count;
      next = list->next;
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
      current = list;
      list = list->next;
      index++;
    }
    current->next = list->next;
    delete list;
  }

  return 0;
}

__glcUniChar* __glcStringList::find(__glcUniChar* inString)
{
  return findIndex(getIndex(inString));
}

__glcUniChar* __glcStringList::findIndex(GLuint inIndex)
{
  __glcStringList *item = this;
  GLuint index = 0;

  if ((inIndex >= count) || (inIndex < 0))
    return NULL;

  while(index != inIndex) {
    item = item->next;
    index++;
  };

  return item->string;
}

GLint __glcStringList::getIndex(__glcUniChar* inString)
{
  __glcStringList *item = this;
  int index = 0;

  if (!string)
    return -1;

  do {
    if (!item->string->compare(inString))
      break;
    item = item->next;
    index++;
  } while (item);

  if (!item)
    return -1;

  return index;
}

GLint __glcStringList::convert(int inType)
{
  GLCchar *room = NULL;
  int size = 0;
  __glcStringList *item = this;

  if (inType == string->type)
    return 0;

  do {
    size = item->string->estimate(inType);
    room = (GLCchar*)malloc(size);
    if (!room)
      return -1;

    item->string->convert(room, inType, size);
    free(item->string->ptr);
    item->string->ptr = room;
    item->string->type = inType;
    item = item->next;
  } while(item);

  return 0;
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
