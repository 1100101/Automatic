ifndef VERBOSE
  Verb := @
endif

CC=gcc
SRC = automatic.c list.c web.c output.c

all: $(SRC) version.h
	$(Verb) $(CC) -Wall -g -funsigned-char -pedantic `xml2-config --cflags` `xml2-config --libs` \
	`curl-config --cflags` `curl-config --libs` $(SRC) -o automatic

memwatch: $(SRC) version.h
	$(Verb) $(CC) $(SRC) memwatch.c -o automatic -DMEMWATCH -Wall -g -pedantic \
	`xml2-config --cflags` `xml2-config --libs` `curl-config --cflags` `curl-config --libs`

clean:
	rm -f *.o automatic version.h

version.h:
	$(Verb) echo '#define SVN_REVISION          "'`svn info | grep "Revision" | awk -F': ' '{print $$2}'`'"' >> version.h
	$(Verb) echo '#define SHORT_VERSION_STRING  "'1.20'"' >> version.h
	$(Verb) echo '#define LONG_VERSION_STRING   "'1.20' ('`svn info | grep "Revision" | awk -F': ' '{print $$2}'`')"' >> version.h
