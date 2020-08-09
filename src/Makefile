#
# Makefile
#

CC = gcc
CPP = g++
LD  = ld

CFLAGS   = -rdynamic -g -finstrument-functions -Wall -pthread 
LDFLAGS  = -rdynamic -lpthread -ldl -lbacktrace -L.
CPP_LDFLAGS  = -rdynamic -liberty -lpthread -ldl -L.

SHARED_LDFLAGS= -shared -fPIC -lpthread -lbacktrace -ldl -L.
SHARED_CPP_LDFLAGS= $(SHARED_LDFLAGS)

all: libtrace.so libtrace++.so

libtrace.so : finstrument.c bt.c
	$(CC) -g -o $@ $+ $(SHARED_LDFLAGS)

libtrace++.so : finstrument.c bt.c
	$(CPP) -o $@ $+ $(SHARED_CPP_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.so
