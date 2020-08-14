#
# Makefile
#

CC = gcc
RM = rm
MV = mv -f

INSTRUMENT_CFLAGS = -finstrument-functions
INSTRUMENT_LDFLAGS= -ldl -rdynamic
CFLAGS = -g $(INSTRUMENT_CFLAGS)
LDFLAGS= $(INSTRUMENT_LDFLAGS)
LDFLAGS_DL= $(INSTRUMENT_LDFLAGS) -ltrace -L../../src

all: hello fork pthread parser tracer_event

hello: hello.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

fork: fork.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

pthread: pthread.c
	$(CC) -o $@ $+ $(CFLAGS)  $(LDFLAGS) -lpthread

tracer_event: tracer_event.c
	$(CC) -o $@ $+ $(CFLAGS)  $(LDFLAGS_DL)

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
	LD_LIBRARY_PATH=../../src:$LD_LIBRARY_PATH ./tracer_event
	$(MV) trace.dat tracer_event.dat

clean:
	$(RM) -f $(OBJS) parser.tab.c parser.tab.h lex.yy.c
	$(RM) -f $(OBJS:%.o=%) parser *.dat
