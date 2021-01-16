#include "bt.h"

int
callback_backtrace(void *vdata, uintptr_t pc)
__attribute__((no_instrument_function));

int
callback_backtrace_pcinfo(void *vdata, uintptr_t pc,
    const char *filename, int lineno, const char *function)
    __attribute__((no_instrument_function));

void init_trace_backtrace()
__attribute__((no_instrument_function));

int trace_backtrace(int depth, struct backtrace_info *symbol_info)
__attribute__((no_instrument_function));

int trace_backtrace_pcinfo(uintptr_t addr, struct backtrace_info *symbol_info)
__attribute__((no_instrument_function));

void *state = NULL;

/* Passed to backtrace callback function.  */

struct bdata
{
    struct backtrace_info *all;
    size_t index;
    size_t max;
};

/* Passed to backtrace_simple callback function.  */

struct sdata
{
    uintptr_t *addrs;
    size_t index;
    size_t max;
};

int
callback_backtrace(void *vdata, uintptr_t pc)
{
    struct sdata *data = (struct sdata *)vdata;

    if (data->index >= data->max)
    {
        fprintf(stderr, "callback_backtrace: callback called too many times\n");
        return 1;
    }

    data->addrs[data->index] = pc;
    ++data->index;

    return 0;
}

int
callback_backtrace_pcinfo(void *vdata, uintptr_t pc,
    const char *filename, int lineno, const char *function)
{
    struct bdata *data = (struct bdata *)vdata;
    struct backtrace_info *p;

    if (data->index >= data->max)
    {
        fprintf(stderr, "callback_backtrace_pcinfo: callback called too many times\n");
        return 1;
    }

    p = &data->all[data->index];
    if (filename == NULL)
        p->filename = NULL;
    else
    {
        p->filename = strdup(filename);
    }
    p->lineno = lineno;
    if (function == NULL)
        p->function = NULL;
    else
    {
        p->function = strdup(function);
    }
    ++data->index;

    return 0;
}

void init_trace_backtrace()
{
    state = backtrace_create_state(NULL, BACKTRACE_SUPPORTS_THREADS, NULL, NULL);
}

int trace_backtrace(int depth, struct backtrace_info *symbol_info)
{
    uintptr_t addrs[20];
    struct sdata data;
    int i, ret;

    data.addrs = &addrs[0];
    data.index = 0;
    data.max = 20;

    struct backtrace_info all[20];
    struct bdata bdata;
    if (state == NULL)
        return 1;
    ret = backtrace_simple((struct backtrace_state *)state, 0, callback_backtrace, NULL, &data);
    if (ret != 0)
        return ret;
    for (i = 0; i < data.index - 1; i++) {
        if (depth != i)
            continue;
        bdata.all = &all[0];
        bdata.index = 0;
        bdata.max = 20;
        ret = backtrace_pcinfo((struct backtrace_state *)state, addrs[i], callback_backtrace_pcinfo,
            NULL, &bdata);
        symbol_info->function = bdata.all[0].function;
        symbol_info->lineno = bdata.all[0].lineno;
        symbol_info->filename = bdata.all[0].filename;
        if (depth == i)
            break;
    }
    return 0;
}


int function_backtrace(int depth, struct backtrace_info *symbol_info)
{
    uintptr_t *addrs;
    struct sdata data;
    int i, ret;

    if (symbol_info == (struct backtrace_info*)NULL) {
        return -1;
    }
    addrs = (uintptr_t *)malloc(sizeof(uintptr_t) * depth);
    data.addrs = &addrs[0];
    data.index = 0;
    data.max = depth;

    struct backtrace_info *all;
    all = (struct backtrace_info *)malloc(sizeof(struct backtrace_info) * depth);
    struct bdata bdata;
    if (state == NULL)
        return 1;
    ret = backtrace_simple((struct backtrace_state *)state, 0, callback_backtrace, NULL, &data);
    if (ret != 0)
        return ret;
    for (i = 0; i < data.index - 1; i++) {
        if (depth != i)
            continue;
        bdata.all = &all[0];
        bdata.index = 0;
        bdata.max = depth;
        ret = backtrace_pcinfo((struct backtrace_state *)state, addrs[i], callback_backtrace_pcinfo,
            NULL, &bdata);
        strcpy(symbol_info[i].function, bdata.all[0].function);
        symbol_info[i].lineno = bdata.all[0].lineno;
        strcpy(symbol_info[i].filename, bdata.all[0].filename);
    }
    free(addrs);
    free(all);
    return 0;
}

int trace_backtrace_pcinfo(uintptr_t addr, struct backtrace_info *symbol_info)
{
    int ret = 0;
    struct backtrace_info all[20];
    struct bdata bdata;
    bdata.all = &all[0];
    bdata.index = 0;
    bdata.max = 20;
    ret = backtrace_pcinfo((struct backtrace_state *)state, addr, callback_backtrace_pcinfo,
        NULL, &bdata);
    symbol_info->function = bdata.all[0].function;
    symbol_info->lineno = bdata.all[0].lineno;
    symbol_info->filename = bdata.all[0].filename;
    return 0;
}

