
## Process this file with automake to produce Makefile.in.

noinst_PROGRAMS = glclogo \
                  tutorial \
                  tutorial2 \
                  unicode \
                  glcdemo \
                  demo

AM_CFLAGS = @PTHREAD_CFLAGS@ \
            @GLUT_CFLAGS@

LDADD = $(top_builddir)/build/libGLC.la \
        @GLUT_LIBS@ \
        @FONTCONFIG_LIBS@ \
        @FREETYPE2_LIBS@ \
        @PTHREAD_LIBS@ \
        @FRIBIDI_LIBS@

demo_SOURCES = demo.c trackball.c trackball.h

clean-generic:
	rm -f *.gcno *.gcda *.gcov
