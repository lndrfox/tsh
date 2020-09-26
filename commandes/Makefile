
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat mkdir

all: $(EXEC)

cp: cp.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp.o: cp.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

cat: cat.o
	$(CC) -o $@ $^ $(LDFLAGS)

cat.o: cat.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

mkdir: mkdir.o
	$(CC) -o $@ $^ $(LDFLAGS)

mkdir.o: mkdir.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
