
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
                 testrender

TESTS = $(noinst_PROGRAMS)

if DEBUGMODE
AM_CFLAGS = -g -DDEBUGMODE
else
AM_CFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif

AM_CFLAGS += @PTHREAD_CFLAGS@ \
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
test6_CFLAGS = $(CFLAGS) -DQUESOGLC_VERSION_MAJOR=@MAJOR@ -DQUESOGLC_VERSION_MINOR=@MINOR@

testcontex_SOURCES = testcontex.c
#testcontex_LDADD = $(LDADD)

testfont_SOURCES = testfont.c
#testfont_LDADD = $(LDADD)

testmaster_SOURCES = testmaster.c
#testmaster_LDADD = $(LDADD)

test7_SOURCES = test7.c
test7_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@
test7_LDADD = @GLUT_LIBS@ $(LDADD)

testrender_SOURCES = testrender.c
testrender_CFLAGS = $(CFLAGS) @GLUT_CFLAGS@
testrender_LDADD = @GLUT_LIBS@ $(LDADD)
