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
OBJECTS = tests/testcommon tests/teststrlst tests/testmaster tests/testfont tests/testrender tests/testcontex

all: $(OBJECTS)

tests/testcommon :  obj/common.o tests/testcommon.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/teststrlst :  obj/common.o obj/strlst.o tests/teststrlst.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testmaster :  obj/common.o obj/context.o obj/strlst.o obj/global.o obj/master.o tests/testmaster.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testfont :  obj/common.o obj/context.o obj/strlst.o obj/global.o obj/master.o  obj/font.o tests/testfont.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testrender :  obj/common.o obj/context.o obj/strlst.o obj/global.o obj/master.o  obj/font.o obj/render.o obj/scalable.o tests/testrender.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testcontex :  obj/common.o obj/context.o obj/strlst.o obj/global.o obj/master.o  obj/font.o tests/testcontex.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/%.o : tests/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	rm -f $(OBJECTS) obj/*.o
	rm -fR docs/html docs/rtf
