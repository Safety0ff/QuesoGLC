# $Id$
CPP = g++
CC = gcc
CPPFLAGS = `freetype-config --cflags`
ifdef DEBUGMODE
CFLAGS = -g
else
CFLAGS = -O2 -fomit-frame-pointer -ffast-math
endif
CFLAGS += -Wall -Iinclude -Isrc -ansi -pedantic-errors
LDFLAGS = -L/usr/X11R6/lib `freetype-config --libs` -lpthread -lGL -lGLU -lglut -lX11 -lXext -lXmu -lgdbm
ifdef DEBUGMODE
LDFLAGS += -lefence
endif
LIBRARY = lib/libglc.a
BUILDER = buildDB
C_SOURCES = context.c font.c global.c master.c measure.c render.c scalable.c \
	  transform.c
CPP_SOURCES = strlst.cpp btree.cpp ocontext.cpp
LIB_OBJECTS = $(addprefix obj/, $(C_SOURCES:.c=.o)) $(addprefix obj/, $(CPP_SOURCES:.cpp=.oo))
DATABASE = database/unicode1.db
OBJECTS = obj/common.o tests/testcommon tests/testmaster tests/testfont \
	tests/testrender tests/testcontex

all: $(DATABASE) $(OBJECTS) $(LIBRARY)

$(DATABASE) : database/$(BUILDER)
	cd database; ./$(BUILDER) *.txt ; cd ..

tests/% :  tests/%.c $(LIBRARY)
	$(CC) $(CFLAGS) $(CPPFLAGS) obj/common.o $^ -o $@ $(LDFLAGS)

obj/%.o : tests/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o : src/%.c
	$(CPP) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	
obj/%.oo : src/%.cpp
	$(CPP) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	
$(LIBRARY) : $(LIB_OBJECTS)
	$(AR) -r $@ $^

database/$(BUILDER) : database/buildDB.c
	$(CC) $(CFLAGS) $^ -o $@ -lgdbm

clean:
	rm -f $(OBJECTS) obj/*.o obj/*.oo
	rm -f $(LIBRARY) database/$(BUILDER)
	rm -f database/*.db
	rm -fR docs/html docs/rtf
