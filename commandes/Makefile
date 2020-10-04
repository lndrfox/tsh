
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat ls

all: $(EXEC)

ls: ls.o
	$(CC) -o $@ $^ $(LDFLAGS)

ls.o: ls.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

cat: cat.o
	$(CC) -o $@ $^ $(LDFLAGS)

cat.o: cat.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
