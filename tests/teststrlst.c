/* $Id$ */
#include "internal.h"
#include <stdio.h>

void display_strlst(FT_List This)
{
  FT_ListNode node = NULL;
  __glcUniChar* ustring = NULL;

  node = This->head;
  while (node) {
    ustring = (__glcUniChar *)node->data;
    printf("%s<end>", (char *)ustring->ptr);
    node = node->next;
  }
  printf("\nLength : %d\n", __glcStrLstLen(This));
}

int main(void)
{
  FT_List sl;
  __glcUniChar s;
  __glcUniChar *ustring = NULL;
  char *string[] = { "string 1", "string 2", "string 3", "string 4",
		     "foobar" };
  int i = 0;
  GLCenum err;

  /* Call in order to initialize the library */
  err = glcGetError();

  sl = __glcStrLstCreate(NULL);

  printf("Append:\n");
  for (i=0; i<3; i++) {
    s.ptr = string[i];
    s.type = GLC_UCS1;
    __glcStrLstAppend(sl, &s);
    display_strlst(sl);
  }

  printf("\nPrepend:\n");
  s.ptr = string[3];
  __glcStrLstPrepend(sl, &s);
  display_strlst(sl);

  printf("\nFindIndex:\n");
  for (i=0; i<5; i++) {
    ustring = __glcStrLstFindIndex(sl, i);
    if (ustring)
      printf("%d : %s<end>\n", i, (char *)ustring->ptr);
    else
      printf("Index %d not found\n", i);
  }
  i = 10;
  ustring = __glcStrLstFindIndex(sl, i);
  if (ustring)
    printf("%d : %s<end>\n", i, (char *)ustring->ptr);
  else
    printf("Index %d not found\n", i);

  printf("\nRemove:\n");
  s.ptr = string[0];
  __glcStrLstRemove(sl, &s);
  printf("Removed %s\n", string[0]);
  display_strlst(sl);

  for (i = 2; i < 5; i++) {
    s.ptr = string[i];
    __glcStrLstRemove(sl, &s);
    printf("Removed %s\n", string[i]);
    display_strlst(sl);
  }

  printf("\n");
  for (i=0; i<2; i++) {
    s.ptr = string[i];
    __glcStrLstAppend(sl, &s);
  }
  s.ptr = string[3];
  __glcStrLstPrepend(sl, &s);
  display_strlst(sl);

  printf("\nRemoveIndex:\n");
  i = 1;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);
  i = 0;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);
  i = 1;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);
  i = 10;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);
  i = 0;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);
  i = 10;
  printf("Remove index %d\n", i);
  __glcStrLstRemoveIndex(sl, i);
  display_strlst(sl);

  printf("\n");
  for (i=0; i<2; i++) {
    s.ptr = string[i];
    __glcStrLstAppend(sl, &s);
  }
  s.ptr = string[3];
  __glcStrLstPrepend(sl, &s);
  display_strlst(sl);

  printf("\nGetIndex:\n");
  for(i = 0; i < 5; i++) {
    s.ptr = string[i];
    printf("%s index %d\n", string[i], __glcStrLstGetIndex(sl, &s));
  }

  __glcStrLstDestroy(sl);

  return 0;
}
