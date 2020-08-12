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
#include <uuid/uuid.h>
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
    int use_sourceline;
    int use_mcheck;
} TRACER_OPTION;


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

#define TRACE_FILE_PATH	            "trace.dat"
#define TRACE_CONF_PATH	            "tracer.conf"
#define MAX_BACK_TRACK_NUM			(5)
#define MAX_LINE_LEN                (1024)

extern void tracer_backtrack(int fd);
extern int writeRingbuffer(int fd);
/**
 * @fn
 * トレース情報にユーザ指定文字列を出力
 * @brief 関数中の特定箇所のトレース情報を得たい場合に使う
 * @param (msg) トレース情報に出力する文字列
 * @return 常に0
 */
extern int tracer_event(const char *msg);
/**
 * @fn
 * トレース情報にユーザ指定文字列を出力
 * @brief 関数中の特定範囲、関数をまたぐ範囲の区間を得たい場合に使う
 * @param (msg) トレース情報に出力する文字列
 * @return 常に0
 * @sa tracer_event_out
 * @details 関数中の特定範囲、関数をまたぐ範囲の区間の開始時に文字列を
 * 付加して呼び出すことで、トレース情報の第1カラムに`UEO`を付与され、
 * 第9カラムに引数の文字列が出力される。
 */
extern int tracer_event_in(const char *msg);
/**
 * @fn
 * トレース情報にユーザ指定文字列を出力
 * @brief 関数中の特定範囲、関数をまたぐ範囲の区間を得たい場合に使う
 * @param (msg) トレース情報に出力する文字列
 * @return 常に0
 * @sa tracer_event_in
 * @details 関数中の特定範囲、関数をまたぐ範囲の区間の終了時に文字列を
 * 付加して呼び出すことで、トレース情報の第1カラムに`UEO`を付与され、
 * 第9カラムに引数の文字列が出力される。
 * @code
 * func()
 * {
 *   tracer_event_in("Started heavy code.");
 *   // do something
 *   tracer_event_out("Ended heavy code.");
 * }
 * @endcode
 */
extern int tracer_event_out(const char *msg);
/**
 * @fn
 * トレース情報にユーザ指定文字列を出力
 * @brief tracer_event_inとほぼ同じだが、コール後に第1引数`id`にUUIDが格納される。
 * @param (id) コール後にUUIDが格納される。
 * @param (msg) トレース情報に出力する文字列
 * @return 常に0
 * @sa tracer_event_out_r
 * @details 関数中の特定範囲、関数をまたぐ範囲の区間の開始時に文字列を
 * 付加して呼び出すことで、トレース情報の第1カラムに`UEI`を付与され、
 * 第8カラムにUUIDが出力される。第9カラムに引数の文字列が出力される。
 * UUIDを識別子に対応するtracer_event_out_rと紐付けることができる。
 * @code
 * func()
 * {
 *   uuid_t id;
 * 
 *   tracer_event_in_r(id, "Started heavy code."); // // ex. id="1b4e28ba-2fa1-11d2-883f-0016d3cca427"
 *   // do something
 *   tracer_event_out_r(id, "Ended heavy code.");
 * }
 * @endcode
 */
extern int tracer_event_in_r(uuid_t id, const char *msg);
/**
 * @fn
 * トレース情報にユーザ指定文字列を出力
 * @brief tracer_event_outとほぼ同じだが、第1引数`id`のUUIDをトレース情報に出力される。
 * @param (id) コール後にUUIDが格納される。
 * @param (msg) トレース情報に出力する文字列
 * @return 常に0
 * @sa tracer_event_in_r
 * @details 関数中の特定範囲、関数をまたぐ範囲の区間の終了時に文字列を
 * 付加して呼び出すことで、トレース情報の第1カラムに`UEO`を付与され、
 * 第8カラムにUUIDが出力される。第9カラムに引数の文字列が出力される。
 * UUIDを識別子に対応するtracer_event_in_rと紐付けることができる。
 */
extern int tracer_event_out_r(uuid_t id, const char *msg);
/**
 * @fn
 * realloc置き換え用関数
 * @brief reallocを行い、トレース情報を出力する.引数、戻り値はreallocと同じ
 * @sa tracer_calloc, tracer_free, tracer_malloc
 * @details REPLACE_GLIBC_ALLOC_FUNCSを定義して、`finstrument.h`をインクルードする。
 * reallocの戻り値、引数がトレース情報に出力される。
 * @code
 * #define REPLACE_GLIBC_ALLOC_FUNCS
 * #include "finstrument.h"
 * 
 * func()
 * {
 *   void *ptr;
 *   ptr = malloc(5); // mallocがtracer_malloc()に置き換わる(realloc, calloc, freeも同様)。
 *   free(ptr);
 * }
 * @endcode
 */
extern void *tracer_realloc(void *ptr, size_t size);
/**
 * @fn
 * calloc置き換え用関数
 * @brief callocを行い、トレース情報を出力する.引数、戻り値はcallocと同じ
 * @sa tracer_free, tracer_malloc, tracer_realloc
 * @details REPLACE_GLIBC_ALLOC_FUNCSを定義して、`finstrument.h`をインクルードする。
 * callocの戻り値、引数がトレース情報に出力される。
 */
extern void *tracer_calloc(size_t nmemb, size_t size);
/**
 * @fn
 * free置き換え用関数
 * @brief callocを行い、トレース情報を出力する.引数、戻り値はfreeと同じ
 * @sa tracer_calloc, tracer_malloc, tracer_realloc
 * @details REPLACE_GLIBC_ALLOC_FUNCSを定義して、`finstrument.h`をインクルードする。
 * freeの戻り値、引数がトレース情報に出力される。
 */
extern void tracer_free(void *ptr);
/**
 * @fn
 * malloc置き換え用関数
 * @brief callocを行い、トレース情報を出力する.引数、戻り値はmallocと同じ
 * @sa tracer_calloc, tracer_free, tracer_realloc
 * @details REPLACE_GLIBC_ALLOC_FUNCSを定義して、`finstrument.h`をインクルードする。
 * mallocの戻り値、引数がトレース情報に出力される。
 */
extern void *tracer_malloc(size_t size);

// REPLACE GLIBC MALLOC 
#ifdef REPLACE_GLIBC_ALLOC_FUNCS
#define malloc(size)    tracer_malloc(size)
#define free(ptr)       tracer_free(ptr)
#define calloc(size)    tracer_calloc(size)
#define realloc(size)   tracer_realloc(size)
#endif //REPLACE_GLIBC_ALLOC_FUNCS

#endif // FINSTRUMENT_H

#ifdef __cplusplus
};
#endif
