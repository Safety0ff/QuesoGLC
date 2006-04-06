#include "GL/glc.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Check that glcGetCharMetric() and glcGetStringCharMetric() return consistant
 * values.
 */

#define EPSILON 1E-5
static char* string = "Hello";
#define CheckError() \
  err = glcGetError(); \
  if (err) { \
    printf("Unexpected error : 0x%X\n", err); \
    return -1; \
  }

int main(void) {
  GLint ctx;
  GLCenum err;
  GLint length = 0;
  GLint i, j;
  GLfloat baseline1[4], baseline2[4];
  GLfloat boundingBox1[8], boundingBox2[8];

  ctx = glcGenContext();
  CheckError();

  glcContext(ctx);
  CheckError();

  length = glcMeasureString(GL_TRUE, string);
  if ((!length) || (length != strlen(string)))
    printf("glcMeasureString() failed to measure %d characters"
	   " (%d measured instead)\n", (int)strlen(string), length);

  CheckError();

  for (i = 0; i < strlen(string); i++) {
    if (!glcGetCharMetric(string[i], GLC_BASELINE, baseline1))
      printf("Failed to measure the baseline of the character %c\n",
	     string[i]);

    CheckError();

    if (!glcGetStringCharMetric(i, GLC_BASELINE, baseline2))
      printf("Failed to get the baseline of the %dth character of the string"
	     " %s\n", i, string);

    CheckError();

    for (j = 0; j < 4; j++) {
      GLfloat v1 = fabs(baseline1[j]);
      GLfloat v2 = fabs(baseline2[j]);
      GLfloat norm = v1 > v2 ? v1 : v2;

      if (fabs(v1 - v2) > EPSILON * norm) {
	printf("Baseline values differ [rank %d char %d] %f (char),"
	       " %f (string)", j, i, v1, v2);
	return -1;
      }
    }

    if (!glcGetCharMetric(string[i], GLC_BOUNDS, boundingBox1))
      printf("Failed to measure the bounding box of the character %c\n",
	     string[i]);

    CheckError();

    if (!glcGetStringCharMetric(i, GLC_BOUNDS, boundingBox2))
      printf("Failed to get the bounding box of the %dth character of the"
	     " string %s\n", i, string);

    CheckError();

    for (j = 0; j < 8; j++) {
      GLfloat v1 = fabs(boundingBox1[j]);
      GLfloat v2 = fabs(boundingBox2[j]);
      GLfloat norm = v1 > v2 ? v1 : v2;

      if (fabs(v1 - v2) > EPSILON * norm) {
	printf("Bounding Box values differ [rank %d char %d] %f (char),"
	       " %f (string)", j, i, v1, v2);
	return -1;
      }
    }
  }

  printf("Tests successfull\n");
  return 0;
}
