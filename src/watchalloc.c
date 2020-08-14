/* This original code written by Sammey. 
 * See https://stackoverflow.com/questions/6083337/overriding-malloc-using-the-ld-preload-mechanism 
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>

#include "base_va.h"

static int libnum = 0;
static int fd;
static PROCESS_MAP_INFO map_info[MAX_LIB_NUM];
static long allocatedSize = 0;

void main_constructor(void)
    __attribute__((constructor));
void main_destructor(void)
    __attribute__((destructor));

static void *(*real_malloc)(size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void (*real_free)(void *) = NULL;

static int alloc_init_pending = 0;

/* Load original allocation routines at first use */
static void alloc_init(void)
{
  alloc_init_pending = 1;
  real_malloc = dlsym(RTLD_NEXT, "malloc");
  real_realloc = dlsym(RTLD_NEXT, "realloc");
  real_calloc = dlsym(RTLD_NEXT, "calloc");
  real_free = dlsym(RTLD_NEXT, "free");
  if (!real_malloc || !real_realloc || !real_calloc || !real_free)
  {
    fputs("alloc.so: Unable to hook allocation!\n", stderr);
    fputs(dlerror(), stderr);
    exit(1);
  }
  alloc_init_pending = 0;
}

#define ZALLOC_MAX 1024
static void *zalloc_list[ZALLOC_MAX];
static size_t zalloc_cnt = 0;

/* dlsym needs dynamic memory before we can resolve the real memory 
 * allocator routines. To support this, we offer simple mmap-based 
 * allocation during alloc_init_pending. 
 * We support a max. of ZALLOC_MAX allocations.
 * 
 * On the tested Ubuntu 16.04 with glibc-2.23, this happens only once.
 */
void *zalloc_internal(size_t size)
{
  if (zalloc_cnt >= ZALLOC_MAX - 1)
  {
    fputs("alloc.so: Out of internal memory\n", stderr);
    return NULL;
  }
  /* Anonymous mapping ensures that pages are zero'd */
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  if (MAP_FAILED == ptr)
  {
    perror("alloc.so: zalloc_internal mmap failed");
    return NULL;
  }
  zalloc_list[zalloc_cnt++] = ptr; /* keep track for later calls to free */
  return ptr;
}

void free(void *ptr)
{
  char buf[MAX_LINE_LEN + 1] = {0};
  void *rptr, *result;
  PROCESS_MAP_INFO *sym;

  if (alloc_init_pending)
  {
    /* Ignore 'free' during initialization and ignore potential mem leaks 
     * On the tested system, this did not happen
     */
    return;
  }
  if (!real_malloc)
  {
    alloc_init();
  }
  for (size_t i = 0; i < zalloc_cnt; i++)
  {
    if (zalloc_list[i] == ptr)
    {
      /* If dlsym cleans up its dynamic memory allocated with zalloc_internal,
       * we intercept and ignore it, as well as the resulting mem leaks.
       * On the tested system, this did not happen
       */
      return;
    }
  }
  rptr = __builtin_return_address(0);
  sym = resolveFuncAddress(map_info, rptr);
  if ((allocatedSize > 0) && (sym != (PROCESS_MAP_INFO *)NULL))
  {
    sprintf(buf, "%s[%lx] free(%p)\n", basename(sym->path),
            (unsigned long)rptr - sym->low + sym->offset, ptr);
    write(fd, buf, strlen(buf));
  }
  real_free(ptr);
}

void *malloc(size_t size)
{
  char buf[MAX_LINE_LEN + 1] = {0};
  void *rptr, *result;
  PROCESS_MAP_INFO *sym;

  if (alloc_init_pending)
  {
    result = zalloc_internal(size);
    rptr = __builtin_return_address(0);
    sym = resolveFuncAddress(map_info, rptr);
    if (sym != (PROCESS_MAP_INFO *)NULL)
    {
      sprintf(buf, "%s[%lx] malloc(0x%zx) = %p\n", basename(sym->path),
              (unsigned long)rptr - sym->low + sym->offset, size, result);
      write(fd, buf, strlen(buf));
    }
    return result;
  }
  if (!real_malloc)
  {
    alloc_init();
  }
  result = real_malloc(size);
  rptr = __builtin_return_address(0);
  sym = resolveFuncAddress(map_info, rptr);
  if (sym != (PROCESS_MAP_INFO *)NULL)
  {
    sprintf(buf, "%s[%lx] malloc(0x%zx) = %p\n", basename(sym->path),
            (unsigned long)rptr - sym->low + sym->offset, size, result);
    write(fd, buf, strlen(buf));
    if (result != (void *)NULL)
    {
      allocatedSize += size;
    }
  }

  return result;
}

void *realloc(void *ptr, size_t size)
{
  char buf[MAX_LINE_LEN + 1] = {0};
  void *rptr, *result;
  PROCESS_MAP_INFO *sym;

  if (alloc_init_pending)
  {
    if (ptr)
    {
      fputs("alloc.so: realloc resizing not supported\n", stderr);
      exit(1);
    }
    result = zalloc_internal(size);
    rptr = __builtin_return_address(0);
    sym = resolveFuncAddress(map_info, rptr);
    if (sym != (PROCESS_MAP_INFO *)NULL)
    {
      sprintf(buf, "%s[%lx] realloc(%p, 0x%zx) = %p\n", basename(sym->path),
              (unsigned long)rptr - sym->low + sym->offset, ptr, size, result);
      write(fd, buf, strlen(buf));
    }
    return result;
  }
  if (!real_malloc)
  {
    alloc_init();
  }
  result = real_realloc(ptr, size);
  rptr = __builtin_return_address(0);
  sym = resolveFuncAddress(map_info, rptr);
  if (sym != (PROCESS_MAP_INFO *)NULL)
  {
    sprintf(buf, "%s[%lx] realloc(%p, 0x%zx) = %p\n", basename(sym->path),
            (unsigned long)rptr - sym->low + sym->offset, ptr, size, result);
    write(fd, buf, strlen(buf));
  }
  return result;
}

void *calloc(size_t nmemb, size_t size)
{
  char buf[MAX_LINE_LEN + 1] = {0};
  void *rptr, *result;
  PROCESS_MAP_INFO *sym;

  if (alloc_init_pending)
  {
    /* Be aware of integer overflow in nmemb*size.
     * Can only be triggered by dlsym */
    result = zalloc_internal(nmemb * size);
    rptr = __builtin_return_address(0);
    sym = resolveFuncAddress(map_info, rptr);
    if (sym != (PROCESS_MAP_INFO *)NULL)
    {
      sprintf(buf, "%s[%lx] calloc(0x%zx, 0x%zx) = %p\n", basename(sym->path),
              (unsigned long)rptr - sym->low + sym->offset, nmemb, size, result);
      write(fd, buf, strlen(buf));
    }
    return result;
  }
  if (!real_malloc)
  {
    alloc_init();
  }
  result = real_calloc(nmemb, size);
  rptr = __builtin_return_address(0);
  sym = resolveFuncAddress(map_info, rptr);
  if (sym != (PROCESS_MAP_INFO *)NULL)
  {
    sprintf(buf, "%s[%lx] calloc(0x%zx, 0x%zx) = %p\n", basename(sym->path),
            (unsigned long)rptr - sym->low + sym->offset, nmemb, size, result);
    write(fd, buf, strlen(buf));
    if (result != (void *)NULL)
    {
      allocatedSize += size;
    }
  }
  return result;
}

void main_constructor(void)
{
  char *mem_trace_log = (char *)NULL;
  mem_trace_log = getenv("MEM_TRACE_LOG");
  if (mem_trace_log == NULL)
  {
    mem_trace_log = "./memtrace.log";
  }
  fd = open(mem_trace_log, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
  if (fd < 0)
  {
    fprintf(stderr, "Error: open(%d) %s\n", errno, strerror(errno));
    exit(-1);
  }
  memset(map_info, 0, sizeof(PROCESS_MAP_INFO) * MAX_LIB_NUM);
  allocatedSize = 0;
  libnum = getProcessMapInfo(map_info);
}

void main_destructor(void)
{
  char buf[MAX_LINE_LEN + 1] = {0};
  sprintf(buf, "allocated size = %ld\n", allocatedSize);
  write(fd, buf, strlen(buf));
  close(fd);
}
