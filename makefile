##
# TaxicabSim
#
# @file
# @version 1
#
CC = -gcc
CFLAGS := -D_GNU_SOURCE -std=c89 -pedantic -g -DDEBUG=0

taxicab: master.o master.h map source taxi cleanall general.h map.h source.h taxi.h
	$(CC) $(CFLAGS) -o taxicab master.c

generator: map.o map.h general.h
	$(CC) $(CFLAGS) -o map map.c

taxi: taxi.o taxi.h general.h general.o
	$(CC) $(CFLAGS) -o taxi taxi.c general.c

source: source.o source.h general.h general.o
	$(CC) $(CFLAGS) -o source source.c general.c

cleanall:
	rm -f *.o
# end
