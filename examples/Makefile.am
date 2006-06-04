noinst_PROGRAMS = glclogo tutorial tutorial2 unicode glcdemo

if DEBUGMODE
AM_CPPFLAGS = -g -DDEBUGMODE
else
AM_CPPFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif

glclogo_SOURCES = glclogo.c $(top_builddir)/include/GL/glc.h
glclogo_LDADD = $(top_builddir)/lib/libGLC.la -lglut

tutorial_SOURCES = tutorial.c $(top_builddir)/include/GL/glc.h
tutorial_LDADD = $(top_builddir)/lib/libGLC.la -lglut

tutorial2_SOURCES = tutorial2.c $(top_builddir)/include/GL/glc.h
tutorial2_LDADD = $(top_builddir)/lib/libGLC.la -lglut

unicode_SOURCES = unicode.c $(top_builddir)/include/GL/glc.h
unicode_LDADD = $(top_builddir)/lib/libGLC.la -lglut

glcdemo_SOURCES = glcdemo.c $(top_builddir)/include/GL/glc.h
glcdemo_LDADD = $(top_builddir)/lib/libGLC.la -lglut
