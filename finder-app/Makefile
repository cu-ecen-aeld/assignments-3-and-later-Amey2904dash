CC=$(CROSS_COMPILE)gcc

default: writer

all: writer.o
	$(CC) -g -Wall writer.o -o writer

writer.o: writer.c
	$(CC) -g -Wall -c -o writer.o writer.c

clean:
	rm -f *.o writer
