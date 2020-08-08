#
# Makefile
#

CC = gcc
RM = rm
MV = mv

OBJS= hello.o fork.o pthread.o
INSTRUMENT_CFLAGS = -finstrument-functions
INSTRUMENT_LDFLAGS= -ldl -rdynamic
CFLAGS = -g $(INSTRUMENT_CFLAGS)
LDFLAGS= $(INSTRUMENT_LDFLAGS)

all: hello fork pthread parser test

hello: hello.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

fork: fork.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

pthread: pthread.c
	$(CC) -o $@ $+ $(CFLAGS)  $(LDFLAGS) -lpthread

%.o: %.y

%.o: %.lex

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: parser.l parser.tab.h
	flex parser.l

parser: lex.yy.c parser.tab.c parser.tab.h
	$(CC) $(CFLAGS) parser.tab.c lex.yy.c -lfl $(LDFLAGS)  -lpthread -o $@

test:
	LD_PRELOAD=../../src/libtrace.so ./hello
	$(MV) trace.dat hello.dat
	LD_PRELOAD=../../src/libtrace.so ./fork
	$(MV) trace.dat fork.dat
	LD_PRELOAD=../../src/libtrace.so ./pthread
	$(MV) trace.dat pthread.dat
	LD_PRELOAD=../../src/libtrace.so ./parser < sample.txt
	$(MV) trace.dat parser.dat

clean:
	$(RM) -f $(OBJS) parser.tab.c parser.tab.h
	$(RM) -f $(OBJS:%.o=%) parser trace.dat
