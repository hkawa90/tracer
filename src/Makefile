#
# Makefile
#

CC = gcc
CPP = g++
LD  = ld

OBJS = test.o finstrument.o
TRACE_OBJS = finstrument.o
CFLAGS   = -rdynamic -g -finstrument-functions -Wall -pthread 
LDFLAGS  = -rdynamic -lpthread -ldl -L.
CPP_LDFLAGS  = -rdynamic -liberty -lpthread -ldl -L.

SHARED_LDFLAGS= -shared -fPIC -lpthread -ldl -L.
SHARED_CPP_LDFLAGS= $(SHARED_LDFLAGS)

all: libtrace.so libtrace++.so

libtrace.so : finstrument.c
	$(CC) -g -o $@ $+ $(SHARED_LDFLAGS)

libtrace++.so : finstrument.c
	$(CPP) -o $@ $+ $(SHARED_CPP_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.so
