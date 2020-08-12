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

all: malloc_leak double_free thread

malloc_leak : malloc_leak.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

double_free : double_free.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS)

thread : thread.c
	$(CC) -o $@ $+ $(CFLAGS) $(LDFLAGS) -lpthread

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
	$(CAT) trace.dat

clean:
	$(RM) malloc_leak double_free thread
	$(RM) *.dat *.log

