# $Id$
CC = gcc
ifdef DEBUGMODE
CFLAGS = -g
else
CFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif
CFLAGS += -Wall -Iinclude -ansi -pedantic-errors `freetype-config --cflags`
LDFLAGS = `freetype-config --libs` -lpthread -lGL -lGLU
OBJECTS = tests/testcommon tests/teststrlst tests/testmaster tests/testfont tests/testrender

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

obj/common.o : tests/common.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	rm -f $(OBJECTS) obj/*.o
