#
# Makefile
#

CC = gcc
RM = rm

OBJS= hello.o fork.o pthread.o
INSTRUMENT_CFLAGS = -finstrument-functions
INSTRUMENT_LDFLAGS= -ldl -rdynamic
CFLAGS = -g $(INSTRUMENT_CFLAGS)
LDFLAGS= $(INSTRUMENT_LDFLAGS)

all: hello fork pthread parser

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

clean:
	$(RM) -f $(OBJS) parser.tab.c parser.tab.h
	$(RM) -f $(OBJS:%.o=%) parser
