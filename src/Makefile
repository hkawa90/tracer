#
# Makefile
#

CC = gcc
CPP = g++
LD  = ld

CFLAGS   = -rdynamic -g -Wall #-pthread 
LDFLAGS  = -rdynamic -lpthread -ldl -lbacktrace -lconfuse -l -L.
CPP_LDFLAGS  = -rdynamic -lpthread -ldl -lbacktrace -lconfuse -L.

SHARED_LDFLAGS= -shared -fPIC -lpthread -lbacktrace -ldl -lconfuse -luuid -L.
SHARED_CPP_LDFLAGS= $(SHARED_LDFLAGS) -liberty

all: libtrace.so libtrace++.so libwatchalloc.so 


libtrace.so : finstrument.c bt.c logger.c ringbuffer.c
	$(CC) -g -o $@ $+ $(SHARED_LDFLAGS)

libwatchalloc.so : watchalloc.c base_va.c
	$(CC) -g -o $@ $+ $(SHARED_LDFLAGS)

libtrace++.so : finstrument.c bt.c logger.c
	$(CPP) -o $@ $+ $(SHARED_CPP_LDFLAGS)

clean:
	rm -f *.o *.so
