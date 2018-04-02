#include "devices.h"
#include <kbd/ps2kbd.h>

int dev_tty_stat(struct stat *destination);
size_t dev_tty_read(char *buffer, size_t count);
size_t dev_tty_write(char *buffer, size_t count);

void dev_tty_init() {

	devfs_device dev;

	dev.stat = dev_tty_stat;
	dev.read = dev_tty_read;
	dev.write = dev_tty_write;

	strcpy(dev.name, "tty");

	devfs_register_device(&dev);

}

int dev_tty_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_tty_read(char *buffer, size_t count) {
	
	// wait the process until \n is read
	// then fill that buffer

	char c = readch();
	size_t s = 0;
	
	while (c != 0 && s < count) {
		buffer[s++] = c;
		c = readch();
	}

	return s;
}

extern void vesa_text_write(const char* data, size_t size);

size_t dev_tty_write(char *buffer, size_t count) {
	vesa_text_write(buffer, count);

	return count;
}