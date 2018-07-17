CC=gcc
CFLAGS=-Wall --pedantic -O0 -g

build: main

run: build
	./main

check: build
	valgrind --tool=memcheck --track-origins=yes --leak-check=yes ./main

main: main.o lib1718.o
	$(CC) -o $@ $^

%.o: %.c lib1718.h Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
