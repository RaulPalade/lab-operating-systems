CFLAGS = -std=c89 -pedantic

all: master node user

master: master.o util.o
	gcc master.o util.o -o master

node: node.o util.o
	gcc node.o util.o -o util

user: user.o util.o
	gcc user.o util.o -o user

master.o: master.c
	gcc -c $(CFLAGS) master.c

node.o: master.c
	gcc -c $(CFLAGS) node.c

user.o: master.c
	gcc -c $(CFLAGS) user.c