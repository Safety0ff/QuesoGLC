#include "GL/glc.h"
#include <pthread.h>

GLint ctx;

void* da_thread(void *arg)
{
  GLCenum err;

  glcContext(ctx);
  err = glcGetError();
  if (err != GLC_STATE_ERROR)
    printf("Unexpected error : 0x%X\n", err);

  return NULL;
}

int main(void)
{
  pthread_t thread;
  GLCenum err;

  ctx = glcGenContext();
  glcContext(ctx);
  err = glcGetError();
  if (err) {
    printf("Unexpected error : 0x%X\n", err);
    return -1;
  }

  if (pthread_create(&thread, NULL, da_thread, NULL)) {
    printf("Failed to create pthread\n");
    return -1;
  }

  if (pthread_join(thread, NULL)) {
    printf("Failed to join the thread\n");
    return -1;
  }

  printf("Test successful!\n");
  return 0;
}
