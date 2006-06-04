noinst_PROGRAMS = test1 test2 test3 test4 test5 test6 \
	testcontex testfont testmaster @TESTS_WITH_GLUT@
EXTRA_PROGRAMS = testrender test7
TESTS = test1 test2 test3 test4 test5 test6 test7 testcontex testfont \
	testmaster

if DEBUGMODE
AM_CPPFLAGS = -g -DDEBUGMODE
else
AM_CPPFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif

test1_SOURCES = test1.c $(top_builddir)/include/GL/glc.h
test1_LDADD = $(top_builddir)/lib/libGLC.la

test2_SOURCES = test2.c $(top_builddir)/include/GL/glc.h
test2_LDADD = $(top_builddir)/lib/libGLC.la

test3_SOURCES = test3.c $(top_builddir)/include/GL/glc.h
test3_LDADD = $(top_builddir)/lib/libGLC.la

test4_SOURCES = test4.c $(top_builddir)/include/GL/glc.h
test4_LDADD = $(top_builddir)/lib/libGLC.la

test5_SOURCES = test5.c $(top_builddir)/include/GL/glc.h
test5_LDADD = $(top_builddir)/lib/libGLC.la

test6_SOURCES = test6.c $(top_builddir)/include/GL/glc.h
test6_LDADD = $(top_builddir)/lib/libGLC.la

test7_SOURCES = test7.c $(top_builddir)/include/GL/glc.h
test7_LDADD = $(top_builddir)/lib/libGLC.la -lglut

testcontex_SOURCES = testcontex.c $(top_builddir)/include/GL/glc.h
testcontex_LDADD = $(top_builddir)/lib/libGLC.la

testfont_SOURCES = testfont.c $(top_builddir)/include/GL/glc.h
testfont_LDADD = $(top_builddir)/lib/libGLC.la

testmaster_SOURCES = testmaster.c $(top_builddir)/include/GL/glc.h
testmaster_LDADD = $(top_builddir)/lib/libGLC.la

testrender_SOURCES = testrender.c $(top_builddir)/include/GL/glc.h
testrender_LDADD = $(top_builddir)/lib/libGLC.la -lglut
