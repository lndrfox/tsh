
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat ls mkdir rmdir cp tsh

all: $(EXEC)

tar_nav.o: tar_nav.c tar_nav.h tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

tar.o: tar.c tar.h
	$(CC) -o $@ -c $< $(CFLAGS)

print.o: print.c print.h
	$(CC) -o $@ -c $< $(CFLAGS)

cd.o: cd.c cd.h
	$(CC) -o $@ -c $< $(CFLAGS)

tsh: tsh.o tar_nav.o print.o cd.o
	$(CC) -o $@ $^ $(LDFLAGS) -lreadline

tsh.o: tsh.c  cd.h 
	$(CC) -o $@ -c $< $(CFLAGS)

ls: ls.o tar.o print.o
	$(CC) -o $@ $^ $(LDFLAGS)

ls.o: ls.c 
	$(CC) -o $@ -c $< $(CFLAGS)

cp: cp.o tar.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp.o: cp.c 
	$(CC) -o $@ -c $< $(CFLAGS)

cat: cat.o tar_nav.o tar.o print.o
	$(CC) -o $@ $^ $(LDFLAGS)

cat.o: cat.c 
	$(CC) -o $@ -c $< $(CFLAGS)

mkdir: mkdir.o tar.o print.o tar_nav.o
	$(CC) -o $@ $^ $(LDFLAGS)

mkdir.o: mkdir.c 
	$(CC) -o $@ -c $< $(CFLAGS)

rmdir: rmdir.o tar.o print.o
	$(CC) -o $@ $^ $(LDFLAGS)

rmdir.o: rmdir.c 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
