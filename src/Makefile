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

SHARED_CFLAGS = -g -Wall -fPIC -I. -rdynamic -g -finstrument-functions -Wall -pthread 
SHARED_LDFLAGS= -shared -fPIC -liberty -lpthread -ldl -L.

all: libtrace.so libtrace++.so 

mytest: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

mycpptest: $(CPP_OBJS)
	$(CPP) -o $@ $(CPP_OBJS) $(CPP_LDFLAGS)

libtrace.so : finstrument.c
	$(CC) -o $@ $+ $(SHARED_LDFLAGS)

libtrace++.o : finstrument.c
	$(CPP) -o $@ $+ $(SHARED_CFLAGS)

libtrace++.so : finstrument.c
	$(CPP) -o $@ $+ $(SHARED_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
