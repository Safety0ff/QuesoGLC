#include <GL/gl.h>
#include "GL/glc.h"
#include <stdio.h>

/* Check GLC routines when no GL context has been bound */

int main(void)
{
	GLint ctx;
	GLint *list;
	GLCenum err;
	int i;

	/* 1. Check that no error is pending */
	err = glcGetError();
	if (err) {
		printf("GLC error pending : 0x%X\n", err);
		return -1;
	}

	/* 2. Check that no context is created */
	list = glcGetAllContexts();
	if (list[0]) {
		printf("Contexts already exist\n");
		free(list);
		return -1;
	}
	free(list);
	
	for (i=0; i<16; i++) {
		if (glcIsContext(i+1)) {
			printf("Context %d already exists\n", i+1);
			return;
		}
	}

	/* 3. Check that we can't destroy contexts that have not been
	 *    created yet.
	 */
	for (i=0; i<16; i++) {
		glcDeleteContext(i+1);
		err = glcGetError();
		if (err != GLC_PARAMETER_ERROR) {
			printf("Unexpected GLC Error : 0x%X\n", err);
			return -1;
		}
	}

	/* 4. Check that 16 contexts can be created */
	for (i=0; i<16; i++) {
		ctx = glcGenContext();
		if (!ctx) {
			printf("GLC error : %d\n", glcGetError());
			return -1;
		}
		if (ctx != i+1) {
			printf("Creation of context %d failed\n", ctx);
			printf("GLC error : %d\n", glcGetError());
			return -1;
		}
	}

	/* 5. Verify that 16 contexts have been generated */
	list = glcGetAllContexts();
	i = 0;
	while (list[i]) {
		if (list[i] != ++i) {
			printf("GLC error : context %d has not been created\n", i);
			return -1;
		}
	}
	free(list);
	
	for (i=0; i<16; i++) {
		if (!glcIsContext(i+1)) {
			printf("Context %d has not been created\n", i+1);
			return -1;
		}
	}

	/* 6. Check that the 17th context creation fails */
	if (!glcGenContext()) {
		err = glcGetError();
		if (err != GLC_RESOURCE_ERROR) {
			printf("Unexpected GLC error : 0x%X\n", err);
			return -1;
		}
	}

	/* 7. Check that there is no current context */
	if (glcGetCurrentContext())
		printf("Unexpected current context %d\n",
			glcGetCurrentContext());

	/* 8. Check that a destroyed context can be reclaimed
	 *    by glcGenContext()
	 */
	glcDeleteContext(5);
	/* Verify that context 5 is not in the context list */
	list = glcGetAllContexts();
	i = 0;
	while (list[i]) {
		if (list[i++] == 5) {
			printf("GLC error : context 5 is not deleted\n");
			return -1;
		}
	}
	if (glcIsContext(5)) {
		printf("Context 5 still exists\n");
		return -1;
	}

	ctx = glcGenContext();
	if (!ctx) {
		printf("GLC Error : 0x%X\n", glcGetError());
		return -1;
	}

	while (list[i]) {
		if (list[i++] == ctx) {
			printf("This context is already defined\n");
			return -1;
		}
	}

	glcContext(ctx);

	printf("Tests successful\n");

	return 0;
}