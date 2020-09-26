
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat cp

all: $(EXEC)

cp: cp.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp.o: cp.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

cat: cat.o
	$(CC) -o $@ $^ $(LDFLAGS)

cat.o: cat.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
