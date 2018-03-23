#ifndef DEVFS
#define DEVFS

#include "vfs.h"

extern struct stat _devfs_stat;

void devfs_init();
int devfs_stat(const char *, struct stat *);
size_t devfs_write(const char *, char *, size_t);
size_t devfs_read(const char *, char *, size_t);

#endif