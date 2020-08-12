#include <stdio.h>
#include <stdlib.h>

#define REPLACE_GLIBC_ALLOC_FUNCS
#include "finstrument.h"

main()
{
    void *ptr;

    ptr = malloc(5);
}