# $Id$
CC = gcc
CPPFLAGS = `freetype-config --cflags`
ifdef DEBUGMODE
CFLAGS = -g
else
CFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif
CFLAGS += -Wall -Iinclude -ansi -pedantic-errors
LDFLAGS = -L/usr/X11R6/lib `freetype-config --libs` -lpthread -lGL -lGLU -lglut -lX11 -lXext -lXmu -lgdbm
ifdef DEBUGMODE
LDFLAGS += -lefence
endif
LIBRARY = lib/libglc.a
BUILDER = buildDB
DATABASE = database/unicode1.db
OBJECTS = obj/common.o tests/testcommon tests/teststrlst tests/testmaster tests/testfont \
	tests/testrender tests/testcontex

all: $(DATABASE) $(OBJECTS) $(LIBRARY)

$(DATABASE) : database/$(BUILDER)
	cd database; ./$(BUILDER) *.txt ; cd ..

tests/% :  tests/%.c $(LIBRARY)
	$(CC) $(CFLAGS) $(CPPFLAGS) obj/common.o $^ -o $@ $(LDFLAGS)

obj/%.o : tests/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	
$(LIBRARY) : obj/strlst.o obj/global.o obj/master.o  obj/font.o obj/render.o \
		obj/scalable.o obj/context.o obj/transform.o obj/measure.o
	$(AR) -r $@ $^

database/$(BUILDER) : database/buildDB.c
	$(CC) $(CFLAGS) $^ -o $@ -lgdbm

clean:
	rm -f $(OBJECTS) obj/*.o
	rm -f $(LIBRARY) database/$(BUILDER)
	rm -f database/*.db
	rm -fR docs/html docs/rtf
