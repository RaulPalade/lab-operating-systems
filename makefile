CC = gcc
CFLAGS = -Wall -Werror -std=c89 -pedantic -g -D_GNU_SOURCE

all: bin/master bin/node bin/user

clean:
	$(RM) -rf bin/*
	$(RM) -rf build/*

build/siglib.o: src/siglib.c include/siglib.h
	$(CC) $(CFLAGS) -c src/siglib.c -o build/siglib.o

build/semlib.o: src/semlib.c include/semlib.h
	$(CC) $(CFLAGS) -c src/semlib.c -o build/semlib.o

build/util.o: src/util.c include/util.h
	$(CC) $(CFLAGS) -c src/util.c -o build/util.o

bin/user: src/user.c build/util.o  build/semlib.o
	$(CC) $(CFLAGS) src/user.c build/util.o build/semlib.o -o bin/user -lm

bin/node: src/node.c build/util.o build/semlib.o
	$(CC) $(CFLAGS) src/node.c build/util.o build/semlib.o -o bin/node -lm

bin/master: app/master.c build/util.o build/semlib.o build/siglib.o
	$(CC) $(CFLAGS) app/master.c build/util.o build/semlib.o build/siglib.o -o bin/master -lm

run: all
	bin/./master