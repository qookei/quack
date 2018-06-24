#include "devices.h"
#include <multiboot.h>
#include <drivers/drv.h>

int dev_mouse_stat(struct stat *destination);
size_t dev_mouse_read(char *buffer, size_t count);
size_t dev_mouse_write(char *buffer, size_t count);

extern multiboot_info_t *mbootinfo;

extern int printf(const char *, ...);

void dev_mouse_init() {

	devfs_device dev;

	dev.stat = dev_mouse_stat;
	dev.read = dev_mouse_read;
	dev.write = dev_mouse_write;

	strcpy(dev.name, "mouse");

	devfs_register_device(&dev);

}

int dev_mouse_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_mouse_read(char *buffer, size_t count) {
	mouse_info_t m;
	if (!drv_mouse_info_global(&m)) return 0;
	
	int32_t x = m.x;
	int32_t y = m.y;
	uint8_t i = m.btn;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	
	if (x >= (signed)mbootinfo->framebuffer_width)  x = mbootinfo->framebuffer_width - 1;
	if (y >= (signed)mbootinfo->framebuffer_height) y = mbootinfo->framebuffer_height - 1;

	uint32_t mbuffer[3] = {0};
	mbuffer[0] = x;
	mbuffer[1] = y;
	mbuffer[2] = i;

	size_t s = count;
	if (s > 12) s = 12;

	memcpy(buffer, mbuffer, s);

	return s;

}

size_t dev_mouse_write(char *buffer, size_t count) {
	(void)buffer;
	(void)count;
	
	return EIO;
}
