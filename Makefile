ifndef VERBOSE
  Verb := @
endif

CC=gcc
SRC = auto_loader.c list.c web.c

all: $(SRC)
	$(Verb) $(CC) -Wall -g -funsigned-char -pedantic `xml2-config --cflags` `xml2-config --libs` \
	`curl-config --cflags` `curl-config --libs` $(SRC) -o auto_loader

memwatch:
	$(Verb) $(CC) $(SRC) memwatch.c -o auto_loader -DMEMWATCH -Wall -g -pedantic \
	`xml2-config --cflags` `xml2-config --libs` `curl-config --cflags` `curl-config --libs`

clean:
	rm -f *.o auto_loader
