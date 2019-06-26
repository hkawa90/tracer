#
# Makefile
#

CC = gcc

OBJS = test.o finstrument.o
CFLAGS   = -rdynamic -g -finstrument-functions -Wall -pthread 
LDFLAGS  = -rdynamic -lpthread -ldl -L.
CPP_LDFLAGS  = -rdynamic -liberty -lpthread -ldl -L.

all: pvtrace mytest

mytest: $(OBJS)
	gcc -o $@ $(OBJS) $(LDFLAGS)

mycpptest: $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $(CPP_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
