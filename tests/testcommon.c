#include <GL/gl.h>

void testQueso(void)
{
    glBegin(GL_LINES);
	glColor3f(1., 0., 0.);
	glVertex2f(0.,0.);
	glVertex2f(100., 100.);
    glEnd();
}
