# $Id$
CC = gcc
ifdef DEBUGMODE
CFLAGS = -g
else
CFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif
CFLAGS += -Wall -Iinclude -ansi -pedantic-errors `freetype-config --cflags`
LDFLAGS = -L/usr/X11R6/lib `freetype-config --libs` -lpthread -lGL -lGLU -lglut -lX11 -lXext -lXmu
ifdef DEBUGMODE
LDFLAGS += -lefence
endif
LIBRARY = lib/libglc.a
OBJECTS = obj/common.o tests/testcommon tests/teststrlst tests/testmaster tests/testfont \
	tests/testrender tests/testcontex

all: $(OBJECTS) $(LIBRARY)

tests/% :  tests/%.c $(LIBRARY)
	$(CC) $(CFLAGS) obj/common.o $^ -o $@ $(LDFLAGS)

obj/%.o : tests/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@
	
$(LIBRARY) : obj/strlst.o obj/global.o obj/master.o  obj/font.o obj/render.o \
		obj/scalable.o obj/context.o obj/transform.o obj/measure.o
	$(AR) -r $@ $^

clean:
	rm -f $(OBJECTS) obj/*.o
	rm -f $(LIBRARY)
	rm -fR docs/html docs/rtf
