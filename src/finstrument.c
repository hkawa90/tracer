#define _FINSTRUMENT_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <confuse.h>
#include <mcheck.h>
#include <string.h>
#include "finstrument.h"
#include "bt.h"
#include "logger.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <libiberty/demangle.h>
#endif

static struct hook_funcs funcs;

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct tracer_info_tag
    {
        struct info *info;
        const char *id;
        int pid;
        struct timespec time;
        struct timespec timeOfThreadProcess;
        char *info1;
        char *info2;
        char *info3;
    } TRACER_INFO;

    /* Function prototypes with attributes */
    int print_traceinfo(int fd, TRACER_INFO *tr)
        __attribute__((no_instrument_function));

    void write_traceinfo(struct info *, const char *status)
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

    struct info *getCaller(int)
        __attribute__((no_instrument_function));

    char *resolveFuncName(uintptr_t addr)
        __attribute__((no_instrument_function));

    void print_def_info(char *buffer, TRACER_INFO *info)
        __attribute__((no_instrument_function));

    int dumpFuncInfo(TRACER_INFO *info)
        __attribute__((no_instrument_function));

    int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                       void *(*start_routine)(void *), void *arg)
        __attribute__((no_instrument_function));

    int pthread_join(pthread_t th, void **thread_return)
        __attribute__((no_instrument_function));

    void pthread_exit(void *retval)
        __attribute__((no_instrument_function, noreturn));

    void exit(int status)
        __attribute__((no_instrument_function, noreturn));

    pid_t fork(void)
        __attribute__((no_instrument_function));

    char *strdup(const char *s)
        __attribute__((no_instrument_function));

    char *strndup(const char *s, size_t n)
        __attribute__((no_instrument_function));

    int isExistFile(const char *)
        __attribute__((no_instrument_function));

    void *tracer_malloc(size_t size)
        __attribute__((no_instrument_function));

    void tracer_free(void *ptr)
        __attribute__((no_instrument_function));

    void *tracer_calloc(size_t nmemb, size_t size)
        __attribute__((no_instrument_function));

    void *tracer_realloc(void *ptr, size_t size)
        __attribute__((no_instrument_function));

    int tracer_event(const char *msg)
        __attribute__((no_instrument_function));

    int tracer_event_in(const char *msg)
        __attribute__((no_instrument_function));

    int tracer_event_out(const char *msg)
        __attribute__((no_instrument_function));

    int tracer_event_in_r(uuid_t id, const char *msg)
        __attribute__((no_instrument_function));

    int tracer_event_out_r(uuid_t id, const char *msg)
        __attribute__((no_instrument_function));

#ifdef __cplusplus
}
#endif

static int fd;
static TRACER trace;

/**
 *
 * @return 存在したら 0以外、存在しなければ 0
*/
int isExistFile(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return (st.st_mode & S_IFMT) == S_IFREG;
}

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
#if 0
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
#endif
}

/**
 * @brief          Write ringbuffer contents to file.
 * @fn             writeRingbuffer
 * @param          (fd) file descriptor
 * @return         return value of write function.
 */
int writeRingbuffer(int fd)
{
#if 0
    TRACER_INFO tr;
    int i, j, ret = 0;

    for (i = 0; i < trace.lookupThreadIDNum; i++)
    {
        int next = (trace.ring[i]->top + 1) % trace.option.max_ringbufferItemNum;
        for (j = next; j < trace.option.max_ringbufferItemNum; j++)
        {
            TRACER_INFO *ti = (TRACER_INFO *)(trace.ring[i]->buffer) + trace.ring[i]->itemSize * j;
            memcpy(&tr, ti, sizeof(TRACER_INFO));
            tr.func = strdup(ti->func);
            if (ti == NULL)
            {
                continue;
            }
            ret = print_traceinfo(fd, &tr);
            if (ret != 0)
                break;
        }
        if (ret != 0)
            break;
    }
    return ret;
#endif
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
#if 0
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
        free(((TRACER_INFO *)ring->buffer + cur_pos)->filename);
    }
    memcpy((TRACER_INFO *)ring->buffer + pos, data, size);
    // update top
    ring->top = (ring->top + 1) % ring->itemNumber;
    return 0;
#endif
}

#if 0
int print_traceinfo(int fd, TRACER_INFO *tr)
{
    int err;
    char buf[MAX_LINE_LEN + 1] = {0};

    snprintf(buf, MAX_LINE_LEN, "%c,%d,%s,%ld,%ld,%ld,%ld,,,",
        tr->status,
        tr->thread_id,
        tr->func,
        tr->time.tv_sec,
        tr->time.tv_nsec,
        tr->timeOfThreadProcess.tv_sec,
        tr->timeOfThreadProcess.tv_nsec);
    if (trace.option.use_sourceline == 1) {
        char lineNoStr[MAX_LINE_LEN + 1];
        strncat(buf, ",", MAX_LINE_LEN);
        strncat(buf, tr->filename, MAX_LINE_LEN);
        strncat(buf, ",", MAX_LINE_LEN);
        snprintf(lineNoStr, MAX_LINE_LEN, "%d", tr->lineNum);
        strncat(buf, lineNoStr, MAX_LINE_LEN);
    }
    strncat(buf, "\n", MAX_LINE_LEN);
    err = write(fd, buf, strnlen(buf, MAX_LINE_LEN));
    free(tr->filename);
    free(tr->func);
    return err;
}
#endif

/**
 * @brief          push trace information to ringbuffer
 * @fn             write_traceinfo
 * @param          (void *this) __cyg_profile_func_ function arg1
 * @param          (void *callsite) __cyg_profile_func_ function arg2
 * @param          (char status) exit or enter
 * @return         void
 */
void write_traceinfo(struct info *symbol_info, const char *status)
{
    TRACER_INFO tr;
    int ret = 0;
#ifdef __cplusplus
    char *demangledStr;
#endif

    memset(&tr, 0, sizeof(tr));
    tr.pid = syscall(SYS_gettid);
    tr.id = status;
    tr.info = symbol_info;

    if (trace.option.use_ringbuffer == 1)
    {
#if 0
        push_ringbuffer(trace.ring[lookupThreadID(tr.thread_id)], &tr, sizeof(tr));
#endif
    }
    else
    {
        dumpFuncInfo(&tr);
    }
}

/**
 * @brief          constructor of main function
 * @fn             main_constructor
 * @return         void
 */
void main_constructor(void)
{
    cfg_opt_t opts[] =
        {
            CFG_INT("use_ringbuffer", 0, CFGF_NONE),
            CFG_INT("use_timestamp", 1, CFGF_NONE),
            CFG_INT("use_cputime", 1, CFGF_NONE),
            CFG_INT("use_backtrack", 0, CFGF_NONE),
            CFG_INT("max_backtrack_num", 10, CFGF_NONE),
            CFG_INT("max_threadNum", 100, CFGF_NONE),
            CFG_INT("max_ringbufferItemNum", 100, CFGF_NONE),
            CFG_INT("use_sourceline", 0, CFGF_NONE),
            CFG_INT("use_mcheck", 0, CFGF_NONE),
            CFG_INT("use_fsync", 0, CFGF_NONE),
            CFG_STR("output_format", "LJSON", CFGF_NONE),
            CFG_INT("use_rotation_log", 0, CFGF_NONE),
            CFG_INT("max_rotation_log", 5, CFGF_NONE),
            CFG_INT("max_rotation_file_size", 1000000, CFGF_NONE),
            CFG_END()};
    cfg_t *cfg;
    const char *tracer_conf = getenv("TRACER_CONF");
    struct sigaction sa_sig;
    char buf[MAX_LINE_LEN + 1] = {0};

    if ((tracer_conf == NULL) || (isExistFile(tracer_conf) == 0))
    {
        tracer_conf = TRACE_CONF_PATH;
    }
    memset(&trace, 0, sizeof(TRACER));
    cfg = cfg_init(opts, CFGF_NONE);
    if (cfg_parse(cfg, tracer_conf) != CFG_PARSE_ERROR)
    {
        trace.option.use_ringbuffer = cfg_getint(cfg, "use_ringbuffer");
        trace.option.use_timestamp = cfg_getint(cfg, "use_timestamp");
        trace.option.use_cputime = cfg_getint(cfg, "use_cputime");
        trace.option.use_backtrack = cfg_getint(cfg, "use_backtrack");
        trace.option.max_backtrack_num = cfg_getint(cfg, "max_backtrack_num");
        trace.option.max_threadNum = cfg_getint(cfg, "max_threadNum");
        trace.option.max_ringbufferItemNum = cfg_getint(cfg, "max_ringbufferItemNum");
        trace.option.use_sourceline = cfg_getint(cfg, "use_sourceline");
        trace.option.use_mcheck = cfg_getint(cfg, "use_mcheck");
        trace.option.use_fsync = cfg_getint(cfg, "use_fsync");
        trace.option.output_format = cfg_getstr(cfg, "output_format");
        if (strcmp(trace.option.output_format, "CSV") == 0)
        {
            trace.option.output_format = "CSV";
        }
        if (strcmp(trace.option.output_format, "LJSON") == 0)
        {
            trace.option.output_format = "LJSON";
        }
        trace.option.use_rotation_log = cfg_getint(cfg, "use_rotation_log");
        trace.option.max_rotation_log = cfg_getint(cfg, "max_rotation_log");
        trace.option.max_rotation_file_size = cfg_getint(cfg, "max_rotation_file_size");
        cfg_free(cfg);
    }
    else
    {
        trace.option.use_ringbuffer = 0;
        trace.option.use_timestamp = 1;
        trace.option.use_cputime = 1;
        trace.option.use_backtrack = 0;
        trace.option.max_backtrack_num = 10;
        trace.option.max_threadNum = 100;
        trace.option.max_ringbufferItemNum = 100;
        trace.option.use_sourceline = 0;
        trace.option.use_mcheck = 0;
        trace.option.use_fsync = 0;
        trace.option.output_format = "LJSON";
        trace.option.use_rotation_log = 0;
        trace.option.max_rotation_log = 5;
        trace.option.max_rotation_file_size = 1000000;
    }
    init_trace_backtrace();
    if (trace.option.use_rotation_log == 1)
    {
        init_logger(trace.option.max_rotation_file_size, trace.option.max_rotation_log, "trace", "dat", S_IWUSR | S_IRUSR);
    }

    funcs.pthread_create = (int (*)(pthread_t *, const pthread_attr_t *, void *(*r)(void *), void *))dlsym(RTLD_NEXT, "pthread_create");
    funcs.pthread_join = (int (*)(pthread_t, void **))dlsym(RTLD_NEXT, "pthread_join");
    funcs.exit = (void (*)(int))dlsym(RTLD_NEXT, "exit");
    funcs.pthread_exit = (void (*)(void *))dlsym(RTLD_NEXT, "pthread_exit");
    funcs.fork = (pid_t(*)(void))dlsym(RTLD_NEXT, "fork");

    changeTraceOption(&trace.option);
    if (trace.option.use_rotation_log == 1)
    {
        open_logger();
    }
    else
    {
        fd = open(TRACE_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
        if (fd < 0)
        {
            fprintf(stderr, "Error: open(%d) %s\n", errno, strerror(errno));
            exit(-1);
        }
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
    if (trace.option.use_mcheck == 1)
    {
        mtrace();
    }
}

/**
 * @brief          deconstructor of main function
 * @fn             main_deconstructor
 * @return         void
 */
void main_deconstructor(void)
{
    if (trace.option.use_rotation_log == 1)
    {
        close_logger();
    }
    else
    {
        close(fd);
    }
    if (trace.option.use_mcheck == 1)
    {
        muntrace();
    }
}

void __cyg_profile_func_enter(void *addr, void *callsite)
{
    struct info symbol_info;
    trace_backtrace(2, &symbol_info);
    write_traceinfo(&symbol_info, "I");
    free(symbol_info.filename);
    free(symbol_info.function);
}

void __cyg_profile_func_exit(void *addr, void *callsite)
{
    struct info symbol_info;
    trace_backtrace(2, &symbol_info);
    write_traceinfo(&symbol_info, "O");
    free(symbol_info.filename);
    free(symbol_info.function);
}

void tracer_backtrack(int fd)
{
#if 0
    int i;
    int thread_id = syscall(SYS_gettid);
    int idx = lookupThreadID(thread_id);

    if (trace.option.use_backtrack)
    {
        for (i = trace.option.max_backtrack_num - 1; i != 0; i--)
        {
            char buf[256];
            int pos = (trace.ring[idx]->top - i) % trace.option.max_ringbufferItemNum;
            TRACER_INFO *ti = (TRACER_INFO *)(trace.ring[idx]->buffer) + trace.ring[idx]->itemSize * pos;
            print_traceinfo(fd, ti);
        }
    }
#endif
}

struct info *getCaller(int depth)
{
    struct info *retVal;
#ifdef __cplusplus
    char *demangledStr;
#endif
    retVal = (struct info *)malloc(sizeof(struct info));
    trace_backtrace(depth + 1, retVal);

#ifdef __cplusplus
    demangledStr = cplus_demangle(retVal->function, 0);
    if (demangledStr != NULL)
    {
        free(retVal->function);
        retVal->function = demangledStr;
    }
#endif

    return retVal;
}

char *resolveFuncName(uintptr_t addr)
{
    struct info symbol_info;
#ifdef __cplusplus
    char *demangledStr;
#endif

    trace_backtrace_pcinfo(addr, &symbol_info);

#ifdef __cplusplus
    demangledStr = cplus_demangle(symbol_info.function, 0);
    if (demangledStr != NULL)
    {
        free(symbol_info.function);
        symbol_info.function = demangledStr;
    }
#endif
    free(symbol_info.filename);
    return symbol_info.function;
}

void print_def_info(char *buffer, TRACER_INFO *info)
{
    char tmp_buf[MAX_LINE_LEN + 1] = {0};
    *buffer = 0;
    if (strcmp(trace.option.output_format, "LJSON") == 0)
    {
        snprintf(tmp_buf, MAX_LINE_LEN, "{ \"id\" : \"%s\", ", info->id);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"pid\": \"%d\", ", info->pid);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"function\": \"%s\", ", info->info->function);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"time1_sec\": \"%ld\", ", info->time.tv_sec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"time1_nsec\": \"%ld\", ", info->time.tv_nsec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"time2_sec\": \"%ld\", ", info->timeOfThreadProcess.tv_sec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"time2_nsec\": \"%ld\", ", info->timeOfThreadProcess.tv_nsec);
        strcat(buffer, tmp_buf);
        if (info->info1 == NULL)
        {
            strcat(buffer, "\"info1\":\"\",");
        }
        else
        {
            strcat(buffer, "\"info1\": \"");
            strcat(buffer, info->info1);
            strcat(buffer, "\"");
            strcat(buffer, ",");
        }
        if (info->info2 == NULL)
        {
            strcat(buffer, "\"info2\": \"\",");
        }
        else
        {
            strcat(buffer, "\"info2\": \"");
            strcat(buffer, info->info2);
            strcat(buffer, "\"");
            strcat(buffer, ",");
        }
        if (info->info3 == NULL)
        {
            strcat(buffer, "\"info3\": \"\",");
        }
        else
        {
            strcat(buffer, "\"info3\": \"");
            strcat(buffer, info->info3);
            strcat(buffer, "\"");
            strcat(buffer, ",");
        }
        if ((trace.option.use_sourceline == 0) || (info->info->filename == NULL))
        {
            strcat(buffer, "\"filename\": \"\",");
        }
        else
        {
            strcat(buffer, "\"filename\": \"");
            strcat(buffer, info->info->filename);
            strcat(buffer, "\"");
            strcat(buffer, ",");
        }
        if (trace.option.use_sourceline == 1)
        {
            snprintf(tmp_buf, MAX_LINE_LEN, "\"lineno\": \"%d\" }\n", info->info->lineno);
        }
        else
        {
            strcat(buffer, "\"lineno\": \"0\" }\n");
        }
    }
    else if (strcmp(trace.option.output_format, "CSV") == 0)
    {
        snprintf(tmp_buf, MAX_LINE_LEN, "%s, ", info->id);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%d, ", info->pid);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%s, ", info->info->function);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%ld, ", info->time.tv_sec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%ld, ", info->time.tv_nsec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%ld, ", info->timeOfThreadProcess.tv_sec);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%ld, ", info->timeOfThreadProcess.tv_nsec);
        strcat(buffer, tmp_buf);
        if (info->info1 == NULL)
        {
            strcat(buffer, ",");
        }
        else
        {
            strcat(buffer, info->info1);
            strcat(buffer, ",");
        }
        if (info->info2 == NULL)
        {
            strcat(buffer, ",");
        }
        else
        {
            strcat(buffer, info->info2);
            strcat(buffer, ",");
        }
        if (info->info3 == NULL)
        {
            strcat(buffer, ",");
        }
        else
        {
            strcat(buffer, info->info3);
            strcat(buffer, ",");
        }
        if ((trace.option.use_sourceline == 0) || (info->info->filename == NULL))
        {
            strcat(buffer, ",");
        }
        else
        {
            strcat(buffer, info->info->filename);
            strcat(buffer, ",");
        }
        if (trace.option.use_sourceline == 1)
        {
            snprintf(tmp_buf, MAX_LINE_LEN, "%d\n", info->info->lineno);
            strcat(buffer, tmp_buf);
        }
        else
        {
            strcat(buffer, "0\n");
        }
    }
}

int dumpFuncInfo(TRACER_INFO *info)
{
    int ret = 0;
    char buf[MAX_LINE_LEN + 1] = {0};
    struct timespec time, timeOfThreadProcess;
#ifdef __cplusplus
    char *demangledStr;
#endif

    pthread_mutex_lock(&trace.trace_write_mutex);
    if (trace.option.use_timestamp == 1)
    {
        if (trace.option.use_cputime == 1)
        {
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &info->timeOfThreadProcess);
        }
        clock_gettime(CLOCK_MONOTONIC, &info->time);
    }
#ifdef __cplusplus
    demangledStr = cplus_demangle(info->info->function, 0);
    if (demangledStr != NULL)
    {
        free(info->info->function);
        info->info->function = demangledStr;
    }
#endif
    print_def_info(buf, info);
    pthread_mutex_unlock(&trace.trace_write_mutex);
    if (trace.option.use_rotation_log == 1)
    {
        write_logger(buf);
    }
    else
    {
        ret = write(fd, buf, strnlen(buf, MAX_LINE_LEN));
    }
    if (trace.option.use_fsync == 1)
    {
        fsync(fd);
    }
    return ret;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    char buf1[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    int ret = 0;
    char *start_routine_symbol = resolveFuncName((uintptr_t)start_routine);

    memset(&info, 0, sizeof(info));
    ret = funcs.pthread_create(thread, attr, start_routine, arg);
    snprintf(buf, MAX_LINE_LEN, "(%d)pthread_create(%s,%ld)", ret, start_routine_symbol, *thread);
    snprintf(buf1, MAX_LINE_LEN, "%ld", *thread);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info1 = buf1;
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    free(start_routine_symbol);
    return ret;
}

int pthread_join(pthread_t th, void **thread_return)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    char buf1[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    int thread_id = syscall(SYS_gettid);
    int ret = 0;

    memset(&info, 0, sizeof(info));

    snprintf(buf, MAX_LINE_LEN, "%ld", th);

    info.id = "EI";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info1 = buf;
    info.info2 = (char *)"pthread_join";
    dumpFuncInfo(&info);

    ret = funcs.pthread_join(th, thread_return);

    snprintf(buf1, MAX_LINE_LEN, "(%d)pthread_join", ret);

    info.id = "EO";
    info.info2 = buf1;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ret;
}

void pthread_exit(void *retval)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = (char *)"pthread_exit";
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    funcs.pthread_exit(retval);
}

void exit(int status)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    snprintf(buf, MAX_LINE_LEN, "exit(%d)", status);

    memset(&info, 0, sizeof(info));
    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    funcs.exit(status);
}

pid_t fork(void)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = (char *)"fork";
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return funcs.fork();
}

char *tracer_strdup(const char *s)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    char *ptr;

    memset(&info, 0, sizeof(info));
    ptr = strdup(s);
    snprintf(buf, MAX_LINE_LEN, "(%p)strdup(%s)", ptr, s);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ptr;
}

char *tracer_strndup(const char *s, size_t n)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    char *ptr;

    memset(&info, 0, sizeof(info));
    ptr = strndup(s, n);
    snprintf(buf, MAX_LINE_LEN, "(%p)strndup(%s, %ld)", ptr, s, n);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ptr;
}

void *tracer_malloc(size_t size)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    void *ptr;

    memset(&info, 0, sizeof(info));
    ptr = malloc(size);
    snprintf(buf, MAX_LINE_LEN, "(%p)malloc(%lu)", ptr, size);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ptr;
}

void tracer_free(void *ptr)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    snprintf(buf, MAX_LINE_LEN, "free(%p)", ptr);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return free(ptr);
}

void *tracer_calloc(size_t nmemb, size_t size)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    void *ptr;

    memset(&info, 0, sizeof(info));
    ptr = calloc(nmemb, size);
    snprintf(buf, MAX_LINE_LEN, "(%p)calloc(%lu,%lu)", ptr, nmemb, size);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ptr;
}

void *tracer_realloc(void *src, size_t size)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;
    void *ptr;

    memset(&info, 0, sizeof(info));
    ptr = realloc(src, size);
    snprintf(buf, MAX_LINE_LEN, "(%p)realloc(%p,%lu)", ptr, src, size);

    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return ptr;
}

int tracer_event(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UE";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return 0;
}

int tracer_event_in_r(uuid_t id, const char *msg)
{
    TRACER_INFO info;
    char *uuid_str = (char *)malloc(strlen(msg) + 37);

    memset(&info, 0, sizeof(info));
    uuid_generate_time_safe(id);
    uuid_unparse_lower(id, uuid_str);

    info.id = "UEI";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info1 = uuid_str;
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    free(uuid_str);
    return 0;
}

int tracer_event_out_r(uuid_t id, const char *msg)
{
    TRACER_INFO info;
    char *uuid_str = (char *)malloc(strlen(msg) + 37);

    memset(&info, 0, sizeof(info));
    uuid_unparse_lower(id, uuid_str);

    info.id = "UEO";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info1 = uuid_str;
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    free(uuid_str);
    return 0;
}

int tracer_event_in(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UEI";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return 0;
}

int tracer_event_out(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UEO";
    info.pid = syscall(SYS_gettid);
    info.info = getCaller(2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(info.info->filename);
    free(info.info->function);
    free(info.info);
    return 0;
}