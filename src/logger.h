#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern void init_logger(int max_size, int max_rotation, const char *base_name, const char *ext_name, mode_t mode);
extern int open_logger(void);
extern int write_logger(const char *msg);
extern int close_logger();

#ifdef __cplusplus
};
#endif

#endif // LOGGER_H