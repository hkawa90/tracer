#define _FINSTRUMENT
#ifdef __cplusplus
extern "C"
{
#endif
#include "finstrument.h"
#include "ringbuffer.h"
#include "bt.h"
#include "logger.h"
#include <limits.h>
#include <confuse.h>
#include <sched.h>              /* for sched_getcpu */
#include <mcheck.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <libiberty/demangle.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct tracer_ {
        TRACER_OPTION option;
        RINGBUFFER **ring;
        int *threadIDTable;
        int lookupThreadIDNum;
        pthread_mutex_t trace_write_mutex;
        pthread_mutex_t trace_lookup_mutex;
    } TRACER;
    
    struct hook_funcs {
        int (*pthread_create)(pthread_t *, const pthread_attr_t *, void *(*r) (void *), void *);
        int (*pthread_join)(pthread_t thread, void **retval);
        void (*exit)(int retval);
        void (*pthread_exit)(void *retval);
        pid_t (*fork)(void);
    };

    struct info
    {
        char filename[MAX_LINE_LEN + 1];
        int lineno;
        char function[MAX_LINE_LEN + 1];
    };

    typedef struct tracer_info_tag
    {
        struct info info;
        const char *id;
        int pid;
        int core_id;
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

    void app_signal_handler(int sig, siginfo_t *info, void *ctx)
        __attribute__((no_instrument_function));

    void tracer_backtrack(int fd)
        __attribute__((no_instrument_function));

    void getCaller(struct info *, int)
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
        __attribute__((no_instrument_function));

    void exit(int status)
        __attribute__((no_instrument_function));

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
static struct hook_funcs funcs;

#define tracer_getcpu()		((trace.option.use_core_id == 1) ? sched_getcpu() : 0)

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
        // TODO realloc
        initRingbuffer(&trace.ring, trace.option.max_threadNum, trace.option.max_ringbufferItemNum, sizeof(TRACER_INFO));
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
    TRACER_INFO tr;
    int i, j, ret = 0;

    for (i = 0; i < trace.lookupThreadIDNum; i++)
    {
        TRACER_INFO *ti = (TRACER_INFO *)nthPinter_ringbuffer(trace.ring[i], i);
        memcpy(&tr, ti, sizeof(TRACER_INFO));
        if (ti == NULL)
        {
            continue;
        }
        ret = print_traceinfo(fd, &tr);
    }
    return ret;
}



int print_traceinfo(int fd, TRACER_INFO *tr)
{
    int err;
    char buf[MAX_LINE_LEN + 1] = {0};

    print_def_info(buf, tr);
    err = write(fd, buf, strnlen(buf, MAX_LINE_LEN));
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
void write_traceinfo(struct info *symbol_info, const char *status)
{
    TRACER_INFO tr;
    int ret = 0;
#ifdef __cplusplus
    char *demangledStr;
#endif

    memset(&tr, 0, sizeof(tr));
    tr.pid = syscall(SYS_gettid);
    tr.core_id = tracer_getcpu();
    tr.id = status;
    memcpy(&tr.info, symbol_info, sizeof(struct info));
    if (trace.option.use_timestamp == 1)
    {
        if (trace.option.use_cputime == 1)
        {
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tr.timeOfThreadProcess);
        }
        clock_gettime(CLOCK_MONOTONIC, &tr.time);
    }
#ifdef __cplusplus
    demangledStr = cplus_demangle(tr.info.function, 0);
    if (demangledStr != NULL)
    {
        strcpy(tr.info.function, demangledStr);
    }
#endif
    if (trace.option.use_ringbuffer == 1)
    {
        int thread_id = syscall(SYS_gettid);
        push_ringbuffer(trace.ring[lookupThreadID(thread_id)], &tr);
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
            CFG_INT("use_core_id", 0, CFGF_NONE),
            CFG_INT("use_fsync", 0, CFGF_NONE),
            CFG_STR("output_format", "LJSON", CFGF_NONE),
            CFG_INT("use_rotation_log", 0, CFGF_NONE),
            CFG_INT("max_rotation_log", 5, CFGF_NONE),
            CFG_INT("max_rotation_file_size", 1000000, CFGF_NONE),
            CFG_END()};
    cfg_t *cfg;
    const char *tracer_conf = getenv("TRACER_CONF");
    const char *tracer_log = getenv("TRACER_LOG");
    struct sigaction sa_sig;
    char buf[MAX_LINE_LEN + 1] = {0};

    if ((tracer_conf == NULL) || (isExistFile(tracer_conf) == 0))
    {
        tracer_conf = TRACE_CONF_PATH;
    }
    if (tracer_log == NULL)
    {
        tracer_log = TRACE_FILE_PATH;
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
        trace.option.use_core_id = cfg_getint(cfg, "use_core_id");
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
        trace.option.use_core_id = 0;
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
        // TODO tracer_log
        init_logger(trace.option.max_rotation_file_size, trace.option.max_rotation_log, tracer_log, "", S_IWUSR | S_IRUSR);
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
        fd = open(tracer_log, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
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
    struct backtrace_info bi;

    trace_backtrace(2, &bi);
    symbol_info.lineno = bi.lineno;
    strcpy(symbol_info.filename, bi.filename);
    strcpy(symbol_info.function, bi.function);
    write_traceinfo(&symbol_info, "I");
}

void __cyg_profile_func_exit(void *addr, void *callsite)
{
    struct info symbol_info;
    struct backtrace_info bi;

    trace_backtrace(2, &bi);
    symbol_info.lineno = bi.lineno;
    strcpy(symbol_info.filename, bi.filename);
    strcpy(symbol_info.function, bi.function);
    write_traceinfo(&symbol_info, "O");
}

void tracer_backtrack(int fd)
{
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
}

void getCaller(struct info * info, int depth)
{
    struct backtrace_info bi;
#ifdef __cplusplus
    char *demangledStr;
#endif
    trace_backtrace(depth + 1, &bi);
    info->lineno = bi.lineno;
    strcpy(info->function, bi.function);
    strcpy(info->filename, bi.filename);
#ifdef __cplusplus
    demangledStr = cplus_demangle(info->function, 0);
    if (demangledStr != NULL)
    {
        strcpy(info->function, demangledStr);
    }
#endif
}

char *resolveFuncName(uintptr_t addr)
{
    struct info symbol_info;
    struct backtrace_info bi;
#ifdef __cplusplus
    char *demangledStr;
#endif

    trace_backtrace_pcinfo(addr, &bi);
    symbol_info.lineno = bi.lineno;
    strcpy(symbol_info.function, bi.function);
    strcpy(symbol_info.filename, bi.filename);

#ifdef __cplusplus
    demangledStr = cplus_demangle(symbol_info.function, 0);
    if (demangledStr != NULL)
    {
        strcpy(symbol_info.function, demangledStr);
    }
#endif
    return strdup(symbol_info.function);
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
        snprintf(tmp_buf, MAX_LINE_LEN, "\"coreid\": \"%d\", ", info->core_id);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "\"function\": \"%s\", ", info->info.function);
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
        if ((trace.option.use_sourceline == 0) || (info->info.filename == NULL))
        {
            strcat(buffer, "\"filename\": \"\",");
        }
        else
        {
            strcat(buffer, "\"filename\": \"");
            strcat(buffer, info->info.filename);
            strcat(buffer, "\"");
            strcat(buffer, ",");
        }
        if (trace.option.use_sourceline == 1)
        {
            snprintf(tmp_buf, MAX_LINE_LEN, "\"lineno\": \"%d\" }\n", info->info.lineno);
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
        snprintf(tmp_buf, MAX_LINE_LEN, "%d, ", info->core_id);
        strcat(buffer, tmp_buf);
        snprintf(tmp_buf, MAX_LINE_LEN, "%s, ", info->info.function);
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
        if ((trace.option.use_sourceline == 0) || (info->info.filename == NULL))
        {
            strcat(buffer, ",");
        }
        else
        {
            strcat(buffer, info->info.filename);
            strcat(buffer, ",");
        }
        if (trace.option.use_sourceline == 1)
        {
            snprintf(tmp_buf, MAX_LINE_LEN, "%d\n", info->info.lineno);
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

    pthread_mutex_lock(&trace.trace_write_mutex);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info1 = buf1;
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info1 = buf;
    info.info2 = (char *)"pthread_join";
    dumpFuncInfo(&info);

    ret = funcs.pthread_join(th, thread_return);

    snprintf(buf1, MAX_LINE_LEN, "(%d)pthread_join", ret);

    info.id = "EO";
    info.info2 = buf1;
    dumpFuncInfo(&info);
    return ret;
}

void pthread_exit(void *retval)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = (char *)"pthread_exit";
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    funcs.exit(status);
}

pid_t fork(void)
{
    char buf[MAX_LINE_LEN + 1] = {0};
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "E";
    info.pid = syscall(SYS_gettid);
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = (char *)"fork";
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = buf;
    dumpFuncInfo(&info);
    return ptr;
}

int tracer_event(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UE";
    info.pid = syscall(SYS_gettid);
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info1 = uuid_str;
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
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
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info1 = uuid_str;
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    free(uuid_str);
    return 0;
}

int tracer_event_in(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UEI";
    info.pid = syscall(SYS_gettid);
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    return 0;
}

int tracer_event_out(const char *msg)
{
    TRACER_INFO info;

    memset(&info, 0, sizeof(info));
    info.id = "UEO";
    info.pid = syscall(SYS_gettid);
    info.core_id = tracer_getcpu();
    getCaller(&info.info, 2);
    info.info2 = (char *)msg;
    dumpFuncInfo(&info);
    return 0;
}
