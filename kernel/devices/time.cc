#include "devices.h"
#include <multiboot.h>
#include <paging/paging.h>

int dev_time_stat(struct stat *destination);
size_t dev_time_read(char *buffer, size_t count);
size_t dev_time_write(char *buffer, size_t count);

extern int printf(const char *, ...);

void dev_time_init() {

	devfs_device dev;

	dev.stat = dev_time_stat;
	dev.read = dev_time_read;
	dev.write = dev_time_write;

	strcpy(dev.name, "time");

	devfs_register_device(&dev);

}

int dev_time_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_size = 4;
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_time_read(char *buffer, size_t count) {
	
	char buf[4];
	*((uint32_t *)buf) = gettime();

	size_t s = count;

	if (s > 4) {
		s = 4;
	}

	memcpy(buffer, buf, s);

	return s;
	
}

size_t dev_time_write(char *buffer, size_t count) {
	(void)buffer;
	(void)count;
	return EIO;
}
