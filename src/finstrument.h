#define HAVE_DECL_BASENAME	1
#ifndef __cplusplus
#define _GNU_SOURCE
#define __USE_GNU
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <dlfcn.h>
#ifdef __cplusplus
#endif

#ifndef FINSTRUMENT_H
#define FINSTRUMENT_H

typedef struct ringbuffer {
    int itemNumber; //
    int itemSize;
    int top;// current position
    void *buffer;
} RINGBUFFER;


typedef struct tracer_option {
    int use_ringbuffer; // if this option is 1, use ringbuffer. When program exit or receive signal(SIGINT), output ringbuerr contents.
    int use_timestamp; //  if this option is 1, add timestamp to trace data.
    int use_cputime; // if this option is 1, timestamp is cputime;
    int use_backtrack;
    int max_backtrack_num;
    int max_threadNum;
    int max_ringbufferItemNum;
} TRACER_OPTION;


typedef struct tracer_ {
    TRACER_OPTION option;
    RINGBUFFER **ring;
    int *threadIDTable;
    int lookupThreadIDNum;
    pthread_mutex_t trace_write_mutex;
    pthread_mutex_t trace_lookup_mutex;
} TRACER;


typedef struct tracer_info {
    struct timespec time;
    struct timespec timeOfThreadProcess;
    int thread_id;

    char status;
    char *func;
} TRACER_INFO;

struct hook_funcs {
    int (*pthread_create)(pthread_t *, const pthread_attr_t *, void *(*r) (void *), void *);
    int (*pthread_join)(pthread_t thread, void **retval);
    void (*exit)(int retval);
    void (*pthread_exit)(void *retval);
    pid_t (*fork)(void);
};

#define TRACE_FILE_PATH	            "trace.dat"
#define TRACE_CONF_PATH	            "tracer.conf"
#define MAX_BACK_TRACK_NUM			(5)
#define MAX_LINE_LEN                (255)
#ifndef _FINSTRUMENT_
extern void tracer_backtrack(int fd);
extern int changeTraceOption(TRACER_OPTION *tp);
extern int writeRingbuffer(int fd);
#endif

#endif // FINSTRUMENT_H

#ifdef __cplusplus
};
#endif
