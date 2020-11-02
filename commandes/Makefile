
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat ls mkdir rmdir cp tsh

all: $(EXEC)

tsh: tsh.o
	$(CC) -o $@ $^ $(LDFLAGS) -lreadline

tsh.o: tsh.c tar.h tar_nav.h cd.h print.h
	$(CC) -o $@ -c $< $(CFLAGS)

ls: ls.o
	$(CC) -o $@ $^ $(LDFLAGS)

ls.o: ls.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

cp: cp.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp.o: cp.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

cat: cat.o
	$(CC) -o $@ $^ $(LDFLAGS)

cat.o: cat.c tar.h tar_nav.h print.h
	$(CC) -o $@ -c $< $(CFLAGS)

mkdir: mkdir.o
	$(CC) -o $@ $^ $(LDFLAGS)

mkdir.o: mkdir.c tar.h print.h tar_nav.h
	$(CC) -o $@ -c $< $(CFLAGS)

rmdir: rmdir.o
	$(CC) -o $@ $^ $(LDFLAGS)

rmdir.o: rmdir.c tar.h print.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
