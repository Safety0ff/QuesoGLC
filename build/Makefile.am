MAJOR = @MAJOR@
MINOR = @MINOR@
REVISION = @REVISION@

if DEBUGMODE
AM_CFLAGS = -g -DDEBUGMODE -Wall -pedantic-errors -Werror
else
AM_CFLAGS = -fomit-frame-pointer -ffast-math
endif

lib_LTLIBRARIES = libGLC.la

libGLC_la_SOURCES = $(top_builddir)/src/context.c \
                    $(top_builddir)/src/database.c \
                    $(top_builddir)/src/except.c \
                    $(top_builddir)/src/except.h \
                    $(top_builddir)/src/font.c \
                    $(top_builddir)/src/global.c \
                    $(top_builddir)/src/master.c \
                    $(top_builddir)/src/master.h \
                    $(top_builddir)/src/measure.c \
                    $(top_builddir)/src/misc.c \
                    $(top_builddir)/src/oarray.c \
                    $(top_builddir)/src/oarray.h \
                    $(top_builddir)/src/ocharmap.c \
                    $(top_builddir)/src/ocharmap.h \
                    $(top_builddir)/src/ocontext.c \
                    $(top_builddir)/src/ocontext.h \
                    $(top_builddir)/src/ofacedesc.c \
                    $(top_builddir)/src/ofacedesc.h \
                    $(top_builddir)/src/ofont.c \
                    $(top_builddir)/src/ofont.h \
                    $(top_builddir)/src/omaster.c \
                    $(top_builddir)/src/omaster.h \
                    $(top_builddir)/src/render.c \
                    $(top_builddir)/src/scalable.c \
                    $(top_builddir)/src/transform.c \
                    $(top_builddir)/src/oglyph.h \
                    $(top_builddir)/src/oglyph.c

libGLC_la_CPPFLAGS = -I$(top_builddir)/src \
                   -DQUESOGLC_VERSION_MAJOR=$(MAJOR) \
                   -DQUESOGLC_VERSION_MINOR=$(MINOR) \
                   @PTHREAD_CFLAGS@ \
                   @FONTCONFIG_CFLAGS@ \
                   @FREETYPE2_CFLAGS@ \
                   @GL_CFLAGS@ \
                   @GLU_CFLAGS@

libGLC_la_LIBADD = @GLU_LIBS@ \
                   @FONTCONFIG_LIBS@ \
                   @FREETYPE2_LIBS@ \
                   @PTHREAD_LIBS@

libGLC_la_LDFLAGS = -version-info 0:2 \
                    -release @PACKAGE_VERSION@
