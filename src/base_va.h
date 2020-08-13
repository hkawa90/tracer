
#include <linux/limits.h>

#ifndef BASE_VA_H
#define BASE_VA_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LINE_LEN (1024)
#define PERMISSION_LEN (4)
#define DEVICE_LEN (5)
#define MAX_LIB_NUM (100)
#define EXE_COL_NUM (7)

typedef struct map_info_tag
{
    unsigned long low;
    unsigned long high;
    char perms[PERMISSION_LEN + 1];
    unsigned int offset;
    char device[DEVICE_LEN + 1];
    unsigned int inode;
    char path[PATH_MAX + 1];
} PROCESS_MAP_INFO;

extern int getProcessMapInfo(PROCESS_MAP_INFO *map_info);
extern PROCESS_MAP_INFO *resolveFuncAddress(PROCESS_MAP_INFO *map_info, void *retrun_address);

#ifdef __cplusplus
}
#endif

#endif