CC=gcc
CFLAGS=-Wall --pedantic -O0 -g

build: main

run: build
	./main

check: build
	valgrind --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all ./main

clean:
	rm main.o lib1718.o main

main: main.o lib1718.o
	$(CC) -o $@ $^

%.o: %.c lib1718.h Makefile tests.h
	$(CC) -c -o $@ $< $(CFLAGS)
