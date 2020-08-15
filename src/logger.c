#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <stdlib.h>

#include "logger.h"

typedef struct logInfo_tag
{
    int max_size;
    int max_rotation;
    int current_size;
    const char *base_name, *ext_name;
    int fd;
    mode_t mode;
    ssize_t (*write)(int fd, const void *buf, size_t count);
} LOG_INFO;

static LOG_INFO logInfo;

int file_stat(const char *filename, mode_t *st_mode, off_t *st_size)
{
    struct stat sb;
    if (stat(filename, &sb) == -1)
    {
        return -1;
    }
    *st_mode = sb.st_mode;
    *st_size = sb.st_size;
    return 0;
}

void init_logger(int max_size, int max_rotation, const char *base_name, const char *ext_name, mode_t mode)
{
    logInfo.max_size = max_size;
    logInfo.max_rotation = max_rotation;
    logInfo.base_name = base_name;
    logInfo.ext_name = ext_name;
    logInfo.fd = -1;
    logInfo.mode = mode;
    logInfo.current_size = 0;
}

/**
 * 通常ファイルかつファイルサイズが0以上を存在するとする
 * @return 存在したら 0以外、存在しなければ 0
*/
static int isExistFile(const char *path)
{
    mode_t st_mode;
    off_t st_size;

    if (file_stat(path, &st_mode, &st_size) == 0)
    {
        if (((st_mode & S_IFMT) == S_IFREG) && (st_size > 0))
        {
            return 1;
        }
    }
    return 0;
}


char *rotation_filename(const char *basename, const char *extname, int number, int max_number)
{
    char buf[PATH_MAX + 1];
    char fmt[100];
    int field_width = 0;
    sprintf(buf, "%d", max_number);
    field_width = strlen(buf);
    sprintf(fmt, "%s%%0%dd.%s", basename, field_width, extname);
    sprintf(buf, fmt, number);
    return strdup(buf);
}

int rotation_logger()
{
    char *path, *path_src;
    char write_path[PATH_MAX + 1];
    mode_t st_mode;
    off_t st_size;
    int ret, i = 0;

    close(logInfo.fd);
    snprintf(write_path, PATH_MAX, "%s.%s", logInfo.base_name, logInfo.ext_name);
    for (i = logInfo.max_rotation; i > 0; i--)
    {
        path = rotation_filename(logInfo.base_name, logInfo.ext_name, i, logInfo.max_rotation);
        path_src = rotation_filename(logInfo.base_name, logInfo.ext_name, i - 1, logInfo.max_rotation);
        if ((i == logInfo.max_rotation) && (file_stat(path, &st_mode, &st_size) == 0) && ((st_mode & S_IFMT) == S_IFREG))
        {
            unlink(path);
        }
        if ((i != logInfo.max_rotation) && (i != 1))
        {
            rename(path_src, path);
        }
        if (i == 1)
        {
            rename(write_path, path);
        }
        free(path);
        free(path_src);
    }
    return 0;
}

int open_logger()
{
    char path[PATH_MAX + 1];
    snprintf(path, PATH_MAX, "%s.%s", logInfo.base_name, logInfo.ext_name);
    if (isExistFile(path) != 0)
    {
        rotation_logger();
    }
    logInfo.fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, logInfo.mode);
    logInfo.current_size = 0;
    return logInfo.fd;
}

int write_logger(const char *msg)
{
    char path[PATH_MAX + 1];
    mode_t st_mode;
    off_t st_size = logInfo.current_size;
    int ret = 0;

    snprintf(path, PATH_MAX, "%s.%s", logInfo.base_name, logInfo.ext_name);

    //    if ((file_stat(path, &st_mode, &st_size) == 0) && ((st_mode & S_IFMT) == S_IFREG))
    {
        if ((st_size + strlen(msg) + 1) > logInfo.max_size)
        {
            // over size
            rotation_logger();
            open_logger();
            ret = write(logInfo.fd, msg, strlen(msg));
            logInfo.current_size += strlen(msg) + 1;
        }
        else
        {
            ret = write(logInfo.fd, msg, strlen(msg));
            logInfo.current_size += strlen(msg) + 1;
        }
    }
    return ret;
}

int close_logger()
{
    return close(logInfo.fd);
}

#if 0
main()
{
    char buf[256];
    long cnt = 0;
    init_logger(100, 3, "mylog", "log", S_IWUSR | S_IRUSR);
    open_logger();
    for (;;)
    {
        usleep(500000);
        sprintf(buf, "Hello...[%ld]\n", cnt);
        write_logger(buf);
        cnt++;
    }
}
#endif
