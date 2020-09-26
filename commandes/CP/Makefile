CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat

all: $

cp: cp.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp.o: cp.c tar.h
	$(CC) -o $@ $^ $<(LDFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
