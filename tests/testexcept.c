#include "GL/glc.h"
#include <stdlib.h>
#include <stdio.h>
#include "except.h"

void distant_throw(void)
{
  printf("Distant throw\n");
  THROW(GLC_MEMORY_EXC);
}

void* my_malloc(size_t size)
{
  void *data = NULL;
  data = malloc(size);
  if (!data)
    THROW(GLC_MEMORY_EXC);
  __glcExceptionPush(free, data);
  return data;
}

int main(void)
{
  __glcException exc = GLC_NO_EXC;
  GLenum err = glcGetError();
  void *data = NULL;

  if (err) {
    printf("Unexpected error 0x%x\n", (int)err);
    return -1;
  }

  TRY
    printf("First Try block\n");
  CATCH(exc)
    printf("First Catch block\nException : %d\n", exc);
  END_CATCH

  printf("Out of first TRY/CATCH block\n");

  TRY
    printf("Second Try block\n");
    THROW(GLC_MEMORY_EXC);
    printf("After throw\n");
  CATCH(exc)
    printf("Second Catch block\nException : %d\n", exc);
  END_CATCH

  printf("Out of second TRY/CATCH block\n");

  TRY
    printf("Third Try block\n");
    data = malloc(10);
    __glcExceptionPush(free, data);
    THROW(GLC_MEMORY_EXC);
    printf("After throw\n");
  CATCH(exc)
    printf("Third Catch block\nException : %d\n", exc);
  END_CATCH

  printf("Out of Third TRY/CATCH block\n");

  TRY
    printf("Fourth Try block\n");
    data = malloc(10);
    __glcExceptionPush(free, data);
    distant_throw();
    printf("After throw\n");
  CATCH(exc)
    printf("Fourth Catch block\nException : %d\n", exc);
  END_CATCH

  printf("Out of Fourth TRY/CATCH block\n");

  TRY
    printf("Fifth Try block\n");
    data = my_malloc(10);
    distant_throw();
    printf("After throw\n");
  CATCH(exc)
    printf("Fifth Catch block\nException : %d\n", exc);
  END_CATCH

  printf("Out of Fifth TRY/CATCH block\n");

  return 0;
}
