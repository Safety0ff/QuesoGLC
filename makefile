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
OBJECTS = tests/testcommon tests/teststrlst tests/testmaster tests/testfont tests/testrender tests/testcontex tests/glclogo

all: $(OBJECTS) $(LIBRARY)

tests/testcommon :  obj/common.o tests/testcommon.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/teststrlst :  obj/common.o tests/teststrlst.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testmaster :  obj/common.o tests/testmaster.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testfont :  obj/common.o tests/testfont.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testrender :  obj/common.o tests/testrender.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/glclogo :  obj/context.o tests/glclogo.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

tests/testcontex :  obj/common.o tests/testcontex.c $(LIBRARY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/%.o : tests/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@
	
$(LIBRARY) : obj/strlst.o obj/global.o obj/master.o  obj/font.o obj/render.o obj/scalable.o obj/context.o obj/transform.o
	$(AR) -r $@ $^

clean:
	rm -f $(OBJECTS) obj/*.o
	rm -f $(LIBRARY)
	rm -fR docs/html docs/rtf
