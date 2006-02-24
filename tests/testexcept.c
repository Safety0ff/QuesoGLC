#include "GL/glc.h"
#include <stdlib.h>
#include <stdio.h>
#include "except.h"

static int distant = 0;

void distant_throw(void)
{
  distant = 1;
  THROW(GLC_MEMORY_EXC);
  printf("Exception not thrown from the distant_throw() function\n");
  exit(-1);
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
  int catched = 0;

  if (err) {
    printf("Unexpected error 0x%x\n", (int)err);
    return -1;
  }

  TRY
    catched = 0;
  CATCH(exc)
    printf("Unexpected catch in the first try/catch block\nException : %d\n", exc);
    return -1;
  END_CATCH

  TRY
    THROW(GLC_MEMORY_EXC);
    printf("Exception not thrown in the 2nd try/catch block\n");
    RETURN(-1);
  CATCH(exc)
    catched = 1;
    if (exc != GLC_MEMORY_EXC) {
      printf("Unexpected exception (%d expected)\n", GLC_MEMORY_EXC);
      return -1;
    }
  END_CATCH

  if (!catched) {
    printf("Failed to catch the exception thrown from the 2nd TRY/CATCH block\n");
    return -1;
  }
  catched = 0;

  TRY
    data = malloc(10);
    __glcExceptionPush(free, data);
    THROW(GLC_MEMORY_EXC);
    printf("Exception not thrown in the 3rd TRY/CATCH block\n");
    return -1;
  CATCH(exc)
    catched = 1;
    if (exc != GLC_MEMORY_EXC) {
      printf("Unexpected exception (%d expected)\n", GLC_MEMORY_EXC);
      return -1;
    }
  END_CATCH

  if (!catched) {
    printf("Failed to catch the exception thrown from the 3rd TRY/CATCH block\n");
    return -1;
  }
  catched = 0;

  TRY
    data = malloc(10);
    __glcExceptionPush(free, data);
    distant_throw();
    printf("Exception not thrown in the 4th TRY/CATCH block\n");
    return -1;
  CATCH(exc)
    catched = 1;
    if (exc != GLC_MEMORY_EXC) {
      printf("Unexpected exception (%d expected)\n", GLC_MEMORY_EXC);
      return -1;
    }
  END_CATCH

  if (!distant) {
    printf("The distant_throw() function has not been called\n");
    return -1;
  }
  distant = 0;
  if (!catched) {
    printf("Failed to catch the exception thrown from the 4th TRY/CATCH block\n");
    return -1;
  }
  catched = 0;

  TRY
    data = my_malloc(10);
    distant_throw();
    printf("Exception not thrown in the 4th TRY/CATCH block\n");
    return -1;
  CATCH(exc)
    catched = 1;
    if (exc != GLC_MEMORY_EXC) {
      printf("Unexpected exception (%d expected)\n", GLC_MEMORY_EXC);
      return -1;
    }
  END_CATCH

  if (!distant) {
    printf("The distant_throw() function has not been called\n");
    return -1;
  }
  distant = 0;
  if (!catched) {
    printf("Failed to catch the exception thrown from the 4th TRY/CATCH block\n");
    return -1;
  }
  catched = 0;

  printf("Tests successfull\n");
  return 0;
}
