CC = gcc
CFLAGS = -Wall -Werror -std=c89 -pedantic -D_GNU_SOURCE

all: bin/master bin/node bin/user

clean:
	$(RM) -rf bin/*
	$(RM) -rf build/*

build/util.o: src/util.c include/util.h
	$(CC) $(CFLAGS) -c src/util.c -o build/util.o

bin/user: src/user.c build/util.o
	$(CC) $(CFLAGS) src/user.c build/util.o -o bin/user

bin/node: src/node.c build/util.o
	$(CC) $(CFLAGS) src/node.c build/util.o -o bin/node

bin/master: app/master.c build/util.o
	$(CC) $(CFLAGS) app/master.c build/util.o -o bin/master

run: all
	bin/./master