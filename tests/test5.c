#include "GL/glc.h"
#include <stdio.h>
#include <math.h>

/* Check the transform routines */

int vectorEquality(GLfloat* vec, GLfloat epsilon)
{
  GLfloat outVec[4];
  int i;

  glcGetfv(GLC_BITMAP_MATRIX, outVec);
  for (i = 0; i < 4; i++)
    if (fabs(outVec[i] - vec[i]) > epsilon) return GL_FALSE;

  return GL_TRUE;
}

int main(void)
{
  GLint ctx = glcGenContext();
  GLfloat ref[4] = {1., 0., 0., 1.};
  
  glcContext(ctx);

  if (!vectorEquality(ref, .00001)) {
    printf("The initial bitmap matrix is not equal to the identity matrix\n");
    return -1;
  }
  
  ref[0] = 1.5;
  ref[1] = -2.5;
  ref[2] = 3.14159;
  ref[3] = 0.;
  glcLoadMatrix(ref);

  if (!vectorEquality(ref, .00001)) {
    printf("glcLoadMatrix() failed\n");
    return -1;
  }
  
  ref[0] = 0.;
  ref[1] = 1.;
  ref[2] = 1.;
  ref[3] = 0.;
  glcMultMatrix(ref);

  ref[0] = 3.14159;
  ref[1] = 0.;
  ref[2] = 1.5;
  ref[3] = -2.5;
  if (!vectorEquality(ref, .00001)) {
    printf("glcMultMatrix() failed\n");
    return -1;
  }
  
  glcScale(-1., 2.);
  ref[0] = -3.14159;
  ref[1] = 0.;
  ref[2] = 3.;
  ref[3] = -5.;
  if (!vectorEquality(ref, .00001)) {
    printf("glcScale() failed\n");
    return -1;
  }
  
  glcRotate(45.);
  ref[0] = -.14159/sqrt(2.);
  ref[2] = 6.14159/sqrt(2.);
  ref[1] = -5./sqrt(2.);
  ref[3] = -5./sqrt(2.);
  if (!vectorEquality(ref, .00001)) {
    printf("glcRotate() failed\n");
    return -1;
  }
  
  glcLoadIdentity();
  ref[0] = 1.;
  ref[1] = 0.;
  ref[2] = 0.;
  ref[3] = 1.;
  if (!vectorEquality(ref, .00001)) {
    printf("glcLoadMatrix() failed\n");
    return -1;
  }
  
  printf("Tests successful\n");
  return 0;
}
