#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <stdio.h>
#include <X11/keysym.h>

extern void testQueso(void);

/* attributes for a single buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
static int attrListDbl[] = { GLX_RGBA,
    GLX_RED_SIZE, 4, 
    GLX_GREEN_SIZE, 4, 
    GLX_BLUE_SIZE, 4, 
    GLX_DEPTH_SIZE, 16,
    None };

typedef struct {
    Display *dpy;
    int screen;
    GLXContext ctx;
    Window win;
    XSetWindowAttributes attr;
    int x, y;
    unsigned int width, height;
    unsigned int depth;    
} glXWin;

int main(void) {
    glXWin win;
    XVisualInfo *vi;
    Colormap cmap;
    char title[] = "QuesoGLC";
    Atom wmDelete;
    Window winDummy;
    unsigned int borderDummy;
    XEvent event;
    Bool done = False;
    
    win.dpy = XOpenDisplay(0);
    win.screen = DefaultScreen(win.dpy);
    
    vi = glXChooseVisual(win.dpy, win.screen, attrListDbl);
    if (!vi)
	return -1;

    win.ctx = glXCreateContext(win.dpy, vi, NULL, True);
    cmap = XCreateColormap(win.dpy, RootWindow(win.dpy, vi->screen), vi->visual, 
	AllocNone);
    win.attr.colormap = cmap;
    win.attr.border_pixel = 0;
    win.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | 
	StructureNotifyMask;
    win.win = XCreateWindow(win.dpy, RootWindow(win.dpy, vi->screen), 0, 0, 640,
	 480, 0, vi->depth, InputOutput, vi->visual, 
	 CWBorderPixel | CWColormap | CWEventMask, &win.attr);
    wmDelete = XInternAtom(win.dpy, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(win.dpy, win.win, &wmDelete, 1);
    XSetStandardProperties(win.dpy, win.win, title, title, None, NULL, 0, NULL);
    XGetGeometry(win.dpy, win.win, &winDummy, &win.x, &win.y, &win.width,
	&win.height, &borderDummy, &win.depth);
    XMapRaised(win.dpy, win.win);

    glXMakeCurrent(win.dpy, win.win, win.ctx);
    
    glViewport(0, 0, win.width, win.height);    /* Reset The Current Viewport And Perspective Transformation */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.325, win.width - 0.325, -0.325, win.height - 0.325);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    testQueso();
    glFlush();

    /* wait for events*/ 
    while (!done)
    {
        /* handle the events in the queue */
        while (XPending(win.dpy) > 0)
        {
            XNextEvent(win.dpy, &event);
            switch (event.type)
            {
                case Expose:
	                if (event.xexpose.count != 0)
	                    break;
/*                     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		    testQueso();
		    glXSwapBuffers(win.dpy, win.win);
 */         	        break;
            case ConfigureNotify:
            /* call resizeGLScene only if our window-size changed */
                if ((event.xconfigure.width != win.width) || 
                    (event.xconfigure.height != win.height))
                {
                    win.width = event.xconfigure.width;
                    win.height = event.xconfigure.height;
		    printf("Resize event\n");
		    if (win.height == 0)    /* Prevent A Divide By Zero If The Window Is Too Small */
			win.height = 1;
		    glViewport(0, 0, win.width, win.height);    /* Reset The Current Viewport And Perspective Transformation */
		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    gluPerspective(45.0f, (GLfloat)win.width / (GLfloat)win.height, 0.1f, 100.0f);
		    glMatrixMode(GL_MODELVIEW);
                }
                break;
            /* exit in case of a mouse button press */
            case ButtonPress:     
                done = True;
                break;
            case KeyPress:
                if (XLookupKeysym(&event.xkey, 0) == XK_Escape)
                {
                    done = True;
                }
                break;
            case ClientMessage:    
                if (*XGetAtomName(win.dpy, event.xclient.message_type) == 
                    *"WM_PROTOCOLS")
                {
                    printf("Exiting sanely...\n");
                    done = True;
                }
                break;
            default:
                break;
            }
        }
/* 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        testQueso();
	glXSwapBuffers(win.dpy, win.win);
 */    }

    glXMakeCurrent(win.dpy, None, NULL);
    glXDestroyContext(win.dpy, win.ctx);
    win.ctx = NULL;
    XCloseDisplay(win.dpy);
    
    return 0;
}
