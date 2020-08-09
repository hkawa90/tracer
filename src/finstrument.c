#define _FINSTRUMENT_
#ifdef __cplusplus
extern "C"
{
    #endif

    #include <stdio.h>
    #include <stdlib.h>
    #include <limits.h>
    #include "finstrument.h"
    #include "bt.h"

    #ifdef __cplusplus
}
#endif


static struct hook_funcs funcs;

#ifdef __cplusplus
extern "C"
{
    #endif
    /* Function prototypes with attributes */
    int print_traceinfo(int fd, TRACER_INFO *tr)
        __attribute__((no_instrument_function));

    void write_traceinfo(struct info, char status)
        __attribute__((no_instrument_function));

    int write_ringbuffer(void *, size_t size)
        __attribute__((no_instrument_function));

    void main_constructor(void)
        __attribute__((no_instrument_function, constructor));

    void main_destructor(void)
        __attribute__((no_instrument_function, destructor));

    void __cyg_profile_func_enter(void *, void *)
        __attribute__((no_instrument_function));

    void __cyg_profile_func_exit(void *, void *)
        __attribute__((no_instrument_function));

    int changeTraceOption(TRACER_OPTION *tp)
        __attribute__((no_instrument_function));

    int lookupThreadID(int thread_id)
        __attribute__((no_instrument_function));

    int writeRingbuffer(int fd)
        __attribute__((no_instrument_function));

    int push_ringbuffer(RINGBUFFER *ring, void *data, size_t size)
        __attribute__((no_instrument_function));

    void app_signal_handler(int sig, siginfo_t *info, void *ctx)
        __attribute__((no_instrument_function));

    void tracer_backtrack(int fd)
        __attribute__((no_instrument_function));

    char *getCaller(int)
        __attribute__((no_instrument_function));

    char *getFuncAddr(uintptr_t addr)
        __attribute__((no_instrument_function));

    int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine)(void *), void *arg)
        __attribute__((no_instrument_function));

    int dumpFuncInfo(int thread_id, const char *caller, const char *hookFuncName)
        __attribute__((no_instrument_function));

    int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine)(void *), void *arg)
        __attribute__((no_instrument_function));

    int pthread_join(pthread_t th, void **thread_return)
        __attribute__((no_instrument_function));

    void pthread_exit(void *retval)
        __attribute__((no_instrument_function));

    void exit(int status)
        __attribute__((no_instrument_function));

    pid_t fork(void)
        __attribute__((no_instrument_function));
    #ifdef __cplusplus
}
#endif

static int fd;
static TRACER trace;

int changeTraceOption(TRACER_OPTION *tp)
{
    int i;

    memcpy(&trace.option, tp, sizeof(TRACER_OPTION));
    // validation check
    if (trace.option.max_threadNum < 1)
    {
        return -1;
    }
    if (trace.option.max_ringbufferItemNum < 1)
    {
        return -2;
        ;
    }
    if (trace.option.use_backtrack)
    {
        trace.option.use_ringbuffer = 1;
        if (trace.option.max_backtrack_num < MAX_BACK_TRACK_NUM)
            trace.option.max_backtrack_num = MAX_BACK_TRACK_NUM;
        trace.option.max_ringbufferItemNum = trace.option.max_backtrack_num;
    }
    if (trace.option.use_ringbuffer)
    {
        // realloc ringbuffer
        trace.ring = (RINGBUFFER **)realloc(trace.ring, sizeof(RINGBUFFER *) * trace.option.max_threadNum);
        memset(trace.ring, 0, sizeof(RINGBUFFER *) * trace.option.max_threadNum);

        for (i = 0; i < trace.option.max_threadNum; i++)
        {
            trace.ring[i] = (RINGBUFFER *)realloc(trace.ring[i], sizeof(RINGBUFFER));
            memset(trace.ring[i], 0, sizeof(RINGBUFFER));

            trace.ring[i]->itemNumber = trace.option.max_ringbufferItemNum;
            trace.ring[i]->itemSize = sizeof(TRACER_INFO);
            trace.ring[i]->top = 0;
            trace.ring[i]->buffer = (void *)realloc(trace.ring[i]->buffer,
                trace.option.max_ringbufferItemNum * trace.ring[i]->itemSize);
        }
        // realloc lookup table for seraching thread id
        trace.threadIDTable = (int *)realloc(trace.threadIDTable, sizeof(int) * trace.option.max_threadNum);
        memset(trace.threadIDTable, 0, sizeof(int) * trace.option.max_threadNum);
    }
    return 0;
}

void app_signal_handler(int sig, siginfo_t *info, void *ctx)
{
    if (trace.option.use_ringbuffer)
    {
        writeRingbuffer(fd);
    }
}

/**
 * @brief          Get index from thread id.
 * @fn             lookupThreadID
 * @param          (thread_id) thread id
 * @return         index
 */
int lookupThreadID(int thread_id)
{
    int i;

    pthread_mutex_lock(&trace.trace_lookup_mutex);
    for (int i = 0; i < trace.lookupThreadIDNum; i++)
    {
        if (trace.threadIDTable[i] == thread_id)
        {
            // found
            return i;
        }
    }
    // add threadIDtable
    if (trace.lookupThreadIDNum >= trace.option.max_threadNum)
    {
        return -1; // overfull
    }
    trace.threadIDTable[trace.lookupThreadIDNum] = thread_id;
    trace.lookupThreadIDNum++;
    pthread_mutex_unlock(&trace.trace_lookup_mutex);
    return trace.lookupThreadIDNum - 1;
}

/**
 * @brief          Write ringbuffer contents to file.
 * @fn             writeRingbuffer
 * @param          (fd) file descriptor
 * @return         return value of write function.
 */
int writeRingbuffer(int fd)
{
    int i, j, ret = 0;

    for (i = 0; i < trace.lookupThreadIDNum; i++)
    {
        int next = (trace.ring[i]->top + 1) % trace.option.max_ringbufferItemNum;
        for (j = next; j < trace.option.max_ringbufferItemNum; j++)
        {
            TRACER_INFO *ti = (TRACER_INFO *)(trace.ring[i]->buffer) + trace.ring[i]->itemSize * j;
            if (ti == NULL)
            {
                continue;
            }
            ret = write(fd, (void *)ti, sizeof(TRACER_INFO));
            if (ret != 0)
                break;
        }
        if (ret != 0)
            break;
        for (j = 0; j < trace.ring[i]->top; j++)
        {
            TRACER_INFO *ti = (TRACER_INFO *)(trace.ring[i]->buffer) + trace.ring[i]->itemSize * j;
            if (ti == NULL)
            {
                continue;
            }
            ret = write(fd, (void *)ti, sizeof(TRACER_INFO));
            if (ret != 0)
                break;
        }
    }
    return ret;
}

/**
 * @brief          push data to ringbuffer
 * @fn             push_ringbuffer
 * @param          (RINGBUFFER *ring) ring buffer
 * @param          (void *data) data
 * @param          (size_t size) write data length.
 * @return         error code.
 */
int push_ringbuffer(RINGBUFFER *ring, void *data, size_t size)
{
    int pos = 0, cur_pos = 0;
    if (ring->itemSize < size)
    {
        return -1;
    }
    if (ring->buffer == NULL)
    {
        return -2;
    }
    pos = ((ring->top + 1) % ring->itemNumber) * ring->itemSize;
    cur_pos = ((ring->top) % ring->itemNumber) * ring->itemSize;
    if (((TRACER_INFO *)ring->buffer + cur_pos)->func != NULL) {
        free(((TRACER_INFO *)ring->buffer + cur_pos)->func);
    }
    memcpy((TRACER_INFO *)ring->buffer + pos, data, size);
    // update top
    ring->top = (ring->top + 1) % ring->itemNumber;
    return 0;
}



int print_traceinfo(int fd, TRACER_INFO *tr)
{
    int err;
    char buf[MAX_LINE_LEN + 1];

    snprintf(buf, MAX_LINE_LEN, "%c,%d,%s,%ld,%ld,%ld,%ld\n",
        tr->status,
        tr->thread_id,
        tr->func,
        tr->time.tv_sec,
        tr->time.tv_nsec,
        tr->timeOfThreadProcess.tv_sec,
        tr->timeOfThreadProcess.tv_nsec);
    err = write(fd, buf, strnlen(buf, MAX_LINE_LEN));
    free(tr->func);
    return err;
}

/**
 * @brief          push trace information to ringbuffer
 * @fn             write_traceinfo
 * @param          (void *this) __cyg_profile_func_ function arg1
 * @param          (void *callsite) __cyg_profile_func_ function arg2
 * @param          (char status) exit or enter
 * @return         void
 */
void write_traceinfo(struct info symbol_info, char status)
{
    TRACER_INFO tr;
    int ret = 0;

    if (trace.option.use_timestamp == 1)
    {
        if (trace.option.use_cputime == 1)
        {
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tr.timeOfThreadProcess);
        }
        clock_gettime(CLOCK_MONOTONIC, &tr.time);
    }
    tr.thread_id = syscall(SYS_gettid);
    tr.status = status;
    tr.func = symbol_info.function;
    if (trace.option.use_ringbuffer == 1)
    {
        push_ringbuffer(trace.ring[lookupThreadID(tr.thread_id)], &tr, sizeof(tr));
    }
    else
    {
        pthread_mutex_lock(&trace.trace_write_mutex);
        print_traceinfo(fd, &tr);
        pthread_mutex_unlock(&trace.trace_write_mutex);
        #if 0
        ret = write(fd, &tr, sizeof(tr));
        if (ret < 0) {
            fprintf(stderr, "Error: writen(%d) %s\n", errno, strerror(errno));
        }
        #endif
    }
}

/**
 * @brief          constructor of main function
 * @fn             main_constructor
 * @return         void
 */
void main_constructor(void)
{
    struct sigaction sa_sig;

    init_trace_backtrace();

    funcs.pthread_create = (int (*)(pthread_t *, const pthread_attr_t *, void *(*r)(void *), void *))dlsym(RTLD_NEXT, "pthread_create");
    funcs.pthread_join = (int (*)(pthread_t, void **))dlsym(RTLD_NEXT, "pthread_join");
    funcs.exit = (void (*)(int))dlsym(RTLD_NEXT, "exit");
    funcs.pthread_exit = (void (*)(void *))dlsym(RTLD_NEXT, "pthread_exit");
    funcs.fork = (pid_t(*)(void))dlsym(RTLD_NEXT, "fork");

    memset(&trace, 0, sizeof(TRACER));
    trace.option.use_ringbuffer = 0; // not used
    trace.option.use_timestamp = 1;
    trace.option.max_threadNum = 100; // not used
    trace.option.use_cputime = 1;

    changeTraceOption(&trace.option);

    fd = open(TRACE_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (fd < 0)
    {
        fprintf(stderr, "Error: open(%d) %s\n", errno, strerror(errno));
        exit(-1);
    }
    memset(&sa_sig, 0, sizeof(sa_sig));
    sa_sig.sa_sigaction = app_signal_handler;
    sa_sig.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa_sig, NULL) < 0)
    {
        exit(1);
    }
    pthread_mutex_init(&trace.trace_write_mutex, NULL);
    pthread_mutex_init(&trace.trace_lookup_mutex, NULL);
}

/**
 * @brief          deconstructor of main function
 * @fn             main_deconstructor
 * @return         void
 */
void main_deconstructor(void)
{
    close(fd);
}

void __cyg_profile_func_enter(void *addr, void *callsite)
{
    struct info symbol_info;
    trace_backtrace(2, &symbol_info);
    write_traceinfo(symbol_info, 'I');
}

void __cyg_profile_func_exit(void *addr, void *callsite)
{
    struct info symbol_info;
    trace_backtrace(2, &symbol_info);
    write_traceinfo(symbol_info, 'O');
}

void tracer_backtrack(int fd)
{
    int i;
    int thread_id = syscall(SYS_gettid);
    int idx = lookupThreadID(thread_id);

    if (trace.option.use_backtrack)
    {
        for (i = 4; i != 0; i--)
        {
            char buf[256];
            int pos = (trace.ring[idx]->top - i) % trace.option.max_ringbufferItemNum;
            TRACER_INFO *ti = (TRACER_INFO *)(trace.ring[idx]->buffer) + trace.ring[idx]->itemSize * pos;
            snprintf(buf, 255, "ThreadID:%d %s\n", ti->thread_id, ti->func);
            write(fd, buf, strnlen(buf, 255));
        }
    }
}

char *getCaller(int depth)
{
    struct info symbol_info;
    trace_backtrace(depth + 1, &symbol_info);
    return symbol_info.function;
}

char *getFuncAddr(uintptr_t addr)
{
    struct info symbol_info;
    trace_backtrace_pcinfo(addr, &symbol_info);
    return symbol_info.function;
}

int dumpFuncInfo(int thread_id, const char *caller, const char *hookFuncName)
{
    char buf[MAX_LINE_LEN + 1];
    struct timespec time, timeOfThreadProcess;

    clock_gettime(CLOCK_MONOTONIC, &time);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &timeOfThreadProcess);
    snprintf(buf, MAX_LINE_LEN, "E,%d,%s,%ld,%ld,%ld,%ld,%s\n",
        thread_id, caller,
        time.tv_sec, time.tv_nsec, timeOfThreadProcess.tv_sec, timeOfThreadProcess.tv_nsec, hookFuncName);
    return write(fd, buf, strnlen(buf, MAX_LINE_LEN));
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
    char buf[MAX_LINE_LEN + 1];
    char funcname[MAX_LINE_LEN + 1];
    int thread_id = syscall(SYS_gettid);
    struct timespec time, timeOfThreadProcess;
    int ret = 0;
    char *symbol = getCaller(2);
    char *start_routine_symbol = getFuncAddr((uintptr_t)start_routine);

    ret = funcs.pthread_create(thread, attr, start_routine, arg);
    snprintf(funcname, MAX_LINE_LEN, "pthread_create %s %ld", start_routine_symbol, *thread);
    dumpFuncInfo(thread_id, symbol, funcname);
    free(symbol);
    free(start_routine_symbol);
    return ret;
}

int pthread_join(pthread_t th, void **thread_return)
{
    char buf[MAX_LINE_LEN + 1];
    char funcname[MAX_LINE_LEN + 1];
    int thread_id = syscall(SYS_gettid);
    int ret = 0;
    char *symbol = getCaller(2);

    snprintf(funcname, MAX_LINE_LEN, "[IN ]pthread_join %ld", th);
    dumpFuncInfo(thread_id, symbol, funcname);
    ret = funcs.pthread_join(th, thread_return);
    snprintf(funcname, MAX_LINE_LEN, "[OUT]pthread_join %ld", th);
    dumpFuncInfo(thread_id, symbol, funcname);
    free(symbol);
    return ret;
}

void pthread_exit(void *retval)
{
    char buf[MAX_LINE_LEN + 1];
    int thread_id = syscall(SYS_gettid);
    char *symbol = getCaller(2);

    dumpFuncInfo(thread_id, symbol, "pthread_exit");
    free(symbol);
    funcs.pthread_exit(retval);
}

void exit(int status)
{
    char buf[MAX_LINE_LEN + 1];
    int thread_id = syscall(SYS_gettid);
    char *symbol = getCaller(2);

    dumpFuncInfo(thread_id, symbol, "exit");
    free(symbol);
    funcs.exit(status);
}

pid_t fork(void)
{
    char buf[MAX_LINE_LEN + 1];
    int thread_id = syscall(SYS_gettid);
    char *symbol = getCaller(2);

    dumpFuncInfo(thread_id, symbol, "fork");
    free(symbol);
    return funcs.fork();
}