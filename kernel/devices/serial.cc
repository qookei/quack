#include "devices.h"
#include <io/serial.h>

int dev_serial_stat(struct stat *destination);
size_t dev_serial_read(char *buffer, size_t count);
size_t dev_serial_write(char *buffer, size_t count);


extern int kprintf(const char *, ...);


void dev_serial_init() {

	devfs_device dev;

	dev.stat = dev_serial_stat;
	dev.read = dev_serial_read;
	dev.write = dev_serial_write;

	strcpy(dev.name, "serial");

	devfs_register_device(&dev);

}

int dev_serial_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_serial_read(char *buffer, size_t count) {
	
	char c = serial_read_byte();
	size_t s = 0;
	
	while (c != 0 && s < count) {
		buffer[s++] = c;
		c = serial_read_byte();
	}

	return s;
}

size_t dev_serial_write(char *buffer, size_t count) {
	for (size_t i = 0; i < count; i++)
		serial_write_byte(buffer[i]);

	return count;
}
