CC=gcc
CFLAGS=-Wall -std=c99 -g
LDFLAGS=-lncursesw -lm

OBJECTS=consolesgf.o
SOURCE=consolesgf.c

consolesgf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o consolesgf $(LDFLAGS)

all:consolesgf

.PHONY: clean
clean:
	rm -f *.o consolesgf
