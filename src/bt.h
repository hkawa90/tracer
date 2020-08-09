#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <backtrace.h>
#include <backtrace-supported.h>

#ifndef TRACE_BACKTRACE_H
#define TRACE_BACKTRACE_H

struct info
{
    char *filename;
    int lineno;
    char *function;
};

#ifdef __cplusplus
extern "C" {
#endif

    extern int trace_backtrace(int depth, struct info *symbol_info) __attribute__((no_instrument_function));
    extern void init_trace_backtrace(void) __attribute__((no_instrument_function));
    extern int trace_backtrace_pcinfo(uintptr_t addr, struct info *symbol_info) __attribute__((no_instrument_function));

#ifdef __cplusplus
} /* End extern "C".  */
#endif


#endif /* !defined(TRACE_BACKTRACE_H) */