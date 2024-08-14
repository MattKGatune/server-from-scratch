CC = gcc

CFLAGS = -g -Wall

http-server: http-server.o

http-server.o:


.PHONY: clean
clean:
	rm -f *.o a.out http-server

.PHONY: all
all: clean default

