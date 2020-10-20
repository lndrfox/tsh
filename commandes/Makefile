
CC=gcc
CFLAGS=-W -Wall
LDFLAGS=
EXEC=cat ls mkdir cd tsh

all: $(EXEC)

tsh: tsh.o
	$(CC) -o $@ $^ $(LDFLAGS) -lreadline

tsh.o: tsh.c tar.h tar_nav.h
	$(CC) -o $@ -c $< $(CFLAGS)

cd: cd.o
	$(CC) -o $@ $^ $(LDFLAGS) 

cd.o: cd.c tar.h tar_nav.h
	$(CC) -o $@ -c $< $(CFLAGS)  

ls: ls.o
	$(CC) -o $@ $^ $(LDFLAGS)

ls.o: ls.c tar.h
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
