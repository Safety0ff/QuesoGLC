/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

int main(void)
{
  int ctx = 0;
  GLCenum error = GLC_NONE;

  error = glcGetError();
  if (error) {
    printf("Unexpected error #0x%X\n", error);
    return -1;
  }
  glcDisable(GLC_AUTO_FONT);
  error = glcGetError();
  if (error != GLC_STATE_ERROR) {
    printf("GLC_STATE_ERROR expected. Error #0x%X instead\n", error);
    return -1;
  }
  error = glcGetError();
  if (error) {
    printf("Error state should have been reset.\nError #0x%X instead\n",
	   error);
    return -1;
  }

  ctx = glcGenContext();
  glcContext(ctx);
  error = glcGetError();
  if (error) {
    printf("Unexpected error #0x%X\n", error);
    return -1;
  }

  if (!glcIsEnabled(GLC_AUTO_FONT)) {
    printf("GLC_AUTO_FONT should be enabled by default\n");
    return -1;
  }
  if (!glcIsEnabled(GLC_GL_OBJECTS)) {
    printf("GLC_GL_OBJECTS should be enabled by default\n");
    return -1;
  }
  if (!glcIsEnabled(GLC_MIPMAP)) {
    printf("GLC_MIPMAP should be enabled by default\n");
    return -1;
  }

  glcDisable(GLC_AUTO_FONT);
  if (glcIsEnabled(GLC_AUTO_FONT)) {
    printf("GLC_AUTO_FONT should be disabled now\n");
    return -1;
  }
  if (!glcIsEnabled(GLC_GL_OBJECTS)) {
    printf("GLC_GL_OBJECTS should be enabled\n");
    return -1;
  }
  if (!glcIsEnabled(GLC_MIPMAP)) {
    printf("GLC_MIPMAP should be enabled\n");
    return -1;
  }

  glcDisable(GLC_GL_OBJECTS);
  if (glcIsEnabled(GLC_AUTO_FONT)) {
    printf("GLC_AUTO_FONT should be disabled now\n");
    return -1;
  }
  if (glcIsEnabled(GLC_GL_OBJECTS)) {
    printf("GLC_GL_OBJECTS should be disbled\n");
    return -1;
  }
  if (!glcIsEnabled(GLC_MIPMAP)) {
    printf("GLC_MIPMAP should be enabled\n");
    return -1;
  }

  error = glcGetError();
  if (error) {
    printf("Unexpected error #0x%X\n", error);
    return -1;
  }
  glcDisable(0);
  error = glcGetError();
  if (error != GLC_PARAMETER_ERROR) {
    printf("GLC_PARAMETER_ERROR expected. Error #0x%X instead\n", error);
    return -1;
  }

  printf("Tests successful!\n");

  return 0;
}
