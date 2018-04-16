#include "devices.h"
#include <multiboot.h>
#include <paging/paging.h>

int dev_uptime_stat(struct stat *destination);
size_t dev_uptime_read(char *buffer, size_t count);
size_t dev_uptime_write(char *buffer, size_t count);

extern int printf(const char *, ...);

void dev_uptime_init() {

	devfs_device dev;

	dev.stat = dev_uptime_stat;
	dev.read = dev_uptime_read;
	dev.write = dev_uptime_write;

	strcpy(dev.name, "uptime");

	devfs_register_device(&dev);

}

int dev_uptime_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_size = 4;
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

extern uint32_t getuptime();

size_t dev_uptime_read(char *buffer, size_t count) {
	
	char buf[4];
	*((uint32_t *)buf) = getuptime();

	size_t s = count;

	if (s > 4) {
		s = 4;
	}

	memcpy(buffer, buf, s);

	return s;
	
}

size_t dev_uptime_write(char *buffer, size_t count) {
	return EIO;
}