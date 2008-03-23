CPATH=/home/kylek/crosscompile/ch3snas
CC=gcc
SRC = xpath1.c list.c memwatch.c

all: list.c list.h xpath1.c
	$(CC) -Wall -funsigned-char -pedantic -I$(CPATH)/include/libxml2 $(SRC) -L$(CPATH)/lib -lxml2 -lm -o auto_loader

