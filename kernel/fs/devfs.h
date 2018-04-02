#ifndef DEVFS
#define DEVFS

#include "vfs.h"

extern struct stat _devfs_stat;

#define DEVFS_DEVICES 16

typedef struct {

	char name[64];

	bool exists;

	int (*stat)(struct stat *);
	size_t (*read)(char *, size_t);
	size_t (*write)(char *, size_t);

} devfs_device;

bool devfs_register_device(devfs_device *dev);

void devfs_init();
int devfs_stat(const char *, struct stat *);
size_t devfs_write(const char *, char *, size_t);
size_t devfs_read(const char *, char *, size_t);

#endif