CC=gcc
CFLAGS=-Wall -O0 -g

build: main

run: build
	./main

main: main.o lib1718.o
	$(CC) -o $@ $^

%.o: %.c lib1718.h Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
