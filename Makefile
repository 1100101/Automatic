CPATH=/home/kylek/crosscompile/ch3snas
CC=gcc
CROSS_CC=arm-linux-gcc
SRC = auto_loader.c list.c memwatch.c

all: $(SRC)
	@ $(CC) -Wall -g -funsigned-char -pedantic `xml2-config --cflags` `xml2-config --libs` $(SRC) -o auto_loader

cross:
	@ $(CROSS_CC) -Wall -g -funsigned-char -pedantic -I$(CPATH)/include/libxml2 $(SRC) \
	-L$(CPATH)/lib -lxml2 -lm -o arm-auto_loader
