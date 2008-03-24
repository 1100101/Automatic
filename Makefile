CPATH=/home/kylek/crosscompile/ch3snas
CC=gcc
SRC = auto_loader.c list.c memwatch.c

all: $(SRC)
	@ $(CC) -Wall -g -funsigned-char -pedantic -I$(CPATH)/include/libxml2 $(SRC) -L$(CPATH)/lib -lxml2 -lm -o auto_loader

