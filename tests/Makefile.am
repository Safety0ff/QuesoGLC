## Process this file with automake to produce Makefile.in.

noinst_PROGRAMS = test4 \
                  @DEBUG_TESTS@ \
                  @TESTS_WITH_GLUT@

EXTRA_PROGRAMS = test1 \
                 test2 \
                 test3 \
                 test5 \
                 test6 \
                 test7 \
                 test8 \
                 test9.1 \
                 test9.2 \
                 test9.3 \
                 test9.4 \
                 test9.5 \
                 test9.6 \
                 test9.7 \
                 test9.8 \
                 test10 \
                 test11.1 \
                 test11.2 \
                 test11.3 \
                 test11.4 \
                 test11.5 \
                 test11.6 \
                 test11.7 \
                 test11.8 \
                 test12 \
                 test13 \
                 test14 \
                 test15 \
                 test16 \
                 test17 \
                 testcontex \
                 testfont \
                 testmaster \
                 testrender

TESTS = $(noinst_PROGRAMS)

AM_CFLAGS = @PTHREAD_CFLAGS@ \
            @GLUT_CFLAGS@

LDADD = $(top_builddir)/build/libGLC.la \
        @GLUT_LIBS@ \
        @FONTCONFIG_LIBS@ \
        @FREETYPE2_LIBS@ \
        @PTHREAD_LIBS@ \
        @FRIBIDI_LIBS@

test6_SOURCES = test6.c
test6_CFLAGS = $(CFLAGS) -DQUESOGLC_VERSION=\"@PACKAGE_VERSION@\" @GLEW_CFLAGS@

test9_1_SOURCES = test9.c
test9_1_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE \
                 -DWITH_GL_OBJECTS

test9_2_SOURCES = test9.c
test9_2_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE

test9_3_SOURCES = test9.c
test9_3_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE \
                 -DWITH_USER_LIST -DWITH_GL_OBJECTS

test9_4_SOURCES = test9.c
test9_4_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE \
                 -DWITH_USER_LIST

test9_5_SOURCES = test9.c
test9_5_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE \
                 -DWITH_GL_OBJECTS

test9_6_SOURCES = test9.c
test9_6_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE

test9_7_SOURCES = test9.c
test9_7_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE \
                 -DWITH_USER_LIST -DWITH_GL_OBJECTS

test9_8_SOURCES = test9.c
test9_8_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE \
                 -DWITH_USER_LIST

test11_1_SOURCES = test11.c
test11_1_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE

test11_2_SOURCES = test11.c
test11_2_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TEXTURE -DWITH_GL_OBJECTS

test11_3_SOURCES = test11.c
test11_3_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE

test11_4_SOURCES = test11.c
test11_4_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_TRIANGLE -DWITH_GL_OBJECTS

test11_5_SOURCES = test11.c
test11_5_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_LINE

test11_6_SOURCES = test11.c
test11_6_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_LINE -DWITH_GL_OBJECTS

test11_7_SOURCES = test11.c
test11_7_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_BITMAP

test11_8_SOURCES = test11.c
test11_8_CFLAGS = $(CFLAGS) -DRENDER_STYLE=GLC_PIXMAP_QSO

clean-generic:
	rm -f *.gcno *.gcda *.gcov
