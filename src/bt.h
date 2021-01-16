#ifndef TRACE_BACKTRACE_H
#define TRACE_BACKTRACE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <backtrace.h>
#include <backtrace-supported.h>


#define MAX_LINE_LEN                (1024)

struct backtrace_info
{
    char *filename;
    int lineno;
    char *function;
};

#ifdef __cplusplus
extern "C" {
#endif

    int trace_backtrace(int depth, struct backtrace_info *symbol_info) __attribute__((no_instrument_function));
    void init_trace_backtrace(void) __attribute__((no_instrument_function));
    int trace_backtrace_pcinfo(uintptr_t addr, struct backtrace_info *symbol_info) __attribute__((no_instrument_function));

#ifdef __cplusplus
} /* End extern "C".  */
#endif


#endif /* !defined(TRACE_BACKTRACE_H) */
