
## Process this file with automake to produce Makefile.in.

noinst_PROGRAMS = test1 \
                  test2 \
                  test3 \
                  test4 \
                  test5 \
                  test6 \
                  testcontex \
                  testfont \
                  testmaster \
                  @TESTS_WITH_GLUT@

EXTRA_PROGRAMS = test7 \
                 test8 \
                 test9.1 \
                 test9.2 \
                 test9.3 \
                 test9.4 \
                 test9.5 \
                 test9.6 \
                 test9.7 \
                 test9.8 \
                 testrender

TESTS = $(noinst_PROGRAMS)

AM_CFLAGS = @PTHREAD_CFLAGS@ \
            @FREETYPE2_CFLAGS@ \
            @FONTCONFIG_CFLAGS@ \
            @GLUT_CFLAGS@

LDADD = $(top_builddir)/build/libGLC.la \
        @GLUT_LIBS@ \
        @FONTCONFIG_LIBS@ \
        @FREETYPE2_LIBS@ \
        @PTHREAD_LIBS@

test1_SOURCES = test1.c

test2_SOURCES = test2.c

test3_SOURCES = test3.c

test4_SOURCES = test4.c

test5_SOURCES = test5.c

test6_SOURCES = test6.c
test6_CFLAGS = $(CFLAGS) -DQUESOGLC_VERSION=\"@PACKAGE_VERSION@\"

testcontex_SOURCES = testcontex.c

testfont_SOURCES = testfont.c

testmaster_SOURCES = testmaster.c

test7_SOURCES = test7.c
test7_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@
test7_LDADD = @GLUT_LIBS@ $(LDADD)

test8_SOURCES = test8.c
test8_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@
test8_LDADD = @GLUT_LIBS@ $(LDADD)

test9_1_SOURCES = test9.c
test9_1_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TEXTURE
test9_1_LDADD = @GLUT_LIBS@ $(LDADD)

test9_2_SOURCES = test9.c
test9_2_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TEXTURE -DWITH_GL_OBJECTS
test9_2_LDADD = @GLUT_LIBS@ $(LDADD)

test9_3_SOURCES = test9.c
test9_3_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TEXTURE -DWITH_USER_LIST
test9_3_LDADD = @GLUT_LIBS@ $(LDADD)

test9_4_SOURCES = test9.c
test9_4_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TEXTURE -DWITH_GL_OBJECTS -DWITH_USER_LIST
test9_4_LDADD = @GLUT_LIBS@ $(LDADD)

test9_5_SOURCES = test9.c
test9_5_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TRIANGLE
test9_5_LDADD = @GLUT_LIBS@ $(LDADD)

test9_6_SOURCES = test9.c
test9_6_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TRIANGLE -DWITH_GL_OBJECTS
test9_6_LDADD = @GLUT_LIBS@ $(LDADD)

test9_7_SOURCES = test9.c
test9_7_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TRIANGLE -DWITH_USER_LIST
test9_7_LDADD = @GLUT_LIBS@ $(LDADD)

test9_8_SOURCES = test9.c
test9_8_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@ -DRENDER_STYLE=GLC_TRIANGLE -DWITH_GL_OBJECTS -DWITH_USER_LIST
test9_8_LDADD = @GLUT_LIBS@ $(LDADD)

testrender_SOURCES = testrender.c
testrender_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@
testrender_LDADD = @GLUT_LIBS@ $(LDADD)
