##
# Transaction Simulator
#
# @file
# @version 1
#
CC = -gcc
CFLAGS := -std=c89 -pedantic

transaction_simulation: master.o master.h node source cleanall node.h user.h util.h
						$(CC) $(CFLAGS) -o transaction_simulator master.c

node: node.o node.h util.h
	$(CC) $(CFLAGS) -o node node.c util.c

user: user.o user.h util.h
	$(CC) $(CFLAGS) -o user user.c util.c

cleanall:
		rm -f *.o
#end