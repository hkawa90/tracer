
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#include "base_va.h"

int getAppFullPath(char *buf, int bufLen, pid_t pid)
{
    char path[PATH_MAX] = {0};

    snprintf(path, PATH_MAX, "/proc/%d/exe", (int)pid); // limits.h
    if (readlink(path, buf, bufLen) < 0)
        return -1;
    else
        return 0;
}

int getProcessMapInfo(PROCESS_MAP_INFO *map_info)
{
    char line[MAX_LINE_LEN + 1];
    FILE *fd;
    pid_t pid;
    char realpath[PATH_MAX + 1] = {0};
    char path[PATH_MAX + 1] = {0};
    int cnt = 0;

    memset((void *)map_info, 0, sizeof(PROCESS_MAP_INFO) * MAX_LIB_NUM);
    pid = getpid();
    snprintf(path, PATH_MAX, "/proc/%d/maps", pid);
    fd = fopen(path, "r");

    while (fgets(line, MAX_LINE_LEN, fd) != NULL)
    {
        int ret = 0;

        ret = sscanf(line, "%lx-%lx %s %x %s %d %s",
                     &map_info[cnt].low,
                     &map_info[cnt].high,
                     map_info[cnt].perms,
                     &map_info[cnt].offset,
                     map_info[cnt].device,
                     &map_info[cnt].inode,
                     map_info[cnt].path);
        if (ret != EXE_COL_NUM)
            continue;
        if (strcmp(map_info[cnt].perms, "r-xp") != 0)
            continue;
        cnt++;
        if (cnt == MAX_LIB_NUM)
            break;
    }
    fclose(fd);
    return cnt;
}

PROCESS_MAP_INFO *resolveFuncAddress(PROCESS_MAP_INFO *map_info, void *retrun_address)
{
    int cnt = 0;

    if (map_info == (PROCESS_MAP_INFO *)NULL)
        return (PROCESS_MAP_INFO *)NULL;
    for (cnt = 0;; cnt++)
    {
        if ((map_info[cnt].low == 0) && (map_info[cnt].high == 0))
            break;
        if ((map_info[cnt].low <= (unsigned long)retrun_address) &&
            (map_info[cnt].high >= (unsigned long)retrun_address))
        {
            return &map_info[cnt];
        }
    }
    return (PROCESS_MAP_INFO *)NULL;
}