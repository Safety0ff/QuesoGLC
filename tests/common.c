#include <GL/glut.h>
#include <stdlib.h>

extern void testQueso(void);

void display(void)
{
}

void reshape(int width, int height)
{
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.325, width - 0.325, -0.325, height - 0.325);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    testQueso();
    glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
    switch(key) {
	case 27: /* Escape Key */
	    exit(0);
	default:
	    break;
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(640, 480);
    glutCreateWindow("QuesoGLC");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
