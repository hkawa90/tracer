#
# Makefile
#

CC = gcc
RM = rm -f
MV = mv -f 
CAT = cat
CP = cp

CFLAGS = -g -finstrument-functions -I../src -DFINSTRUMENT
LDFLAGS = -ltrace -L../src
LDFLAGS_NOFINSTRUMENT =

all: malloc_leak double_free thread factorial Hello tracer_event sHello strdup_leak smalloc_leak

malloc_leak : malloc_leak.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

smalloc_leak : smalloc_leak.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS_NOFINSTRUMENT)

strdup_leak : strdup_leak.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS_NOFINSTRUMENT)

double_free : double_free.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

thread : thread.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS) -lpthread

factorial : factorial.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

Hello : Hello.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

sHello : Hello.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS_NOFINSTRUMENT)

tracer_event : tracer_event.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

test:
	$(RM) *.dat mtrace.log tracer.conf
	$(CP) mtrace_tracer.conf tracer.conf
	-MALLOC_TRACE="mtrace.log" LD_LIBRARY_PATH=../src ./malloc_leak
	-mtrace ./malloc_leak mtrace.log
	$(CAT) mtrace.log
	$(RM) mtrace.log
	-MALLOC_TRACE="mtrace.log" LD_LIBRARY_PATH=../src ./double_free
	-mtrace ./double_free mtrace.log
	$(CAT) mtrace.log
	$(RM) mtrace.log
	-LD_LIBRARY_PATH=../src ./thread
	$(MV) trace.dat thread.dat
	-LD_LIBRARY_PATH=../src ./factorial
	$(MV) trace.dat factorial.dat
	$(CP) csv.conf tracer.conf
	-LD_LIBRARY_PATH=../src ./Hello
	$(MV) trace.dat Hello_csv.dat
	$(RM) tracer.conf
	-LD_LIBRARY_PATH=../src ./Hello
	$(MV) trace.dat Hello.dat
	-LD_LIBRARY_PATH=../src ./tracer_event
	$(MV) trace.dat tracer_event.dat

clean:
	$(RM) malloc_leak double_free thread
	$(RM) *.dat *.log

