#include "devices.h"
#include <multiboot.h>
#include <paging/paging.h>

int dev_initrd_stat(struct stat *destination);
size_t dev_initrd_read(char *buffer, size_t count);
size_t dev_initrd_write(char *buffer, size_t count);

size_t initrd_sz;
char *initrd;

extern multiboot_info_t *mbootinfo;

extern int printf(const char *, ...);

void dev_initrd_init() {
	multiboot_module_t* p = (multiboot_module_t*) (0xC0000000 + mbootinfo->mods_addr);

	uint32_t sta = p->mod_start;
	uint32_t end = p->mod_end - 1;

	for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {
		map_page((void*)((sta & 0xFFFFF000) + (i * 0x1000)), (void*)(0xE0000000 + (i * 0x1000)), 0x3);
	}

	initrd_sz = end - sta + 1;
	initrd = (char *)kmalloc(initrd_sz);

	memcpy(initrd, (void *)0xE0000000, initrd_sz);

	for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {
		unmap_page((void*)(0xE0000000 + (i * 0x1000)));
	}

	devfs_device dev;

	dev.stat = dev_initrd_stat;
	dev.read = dev_initrd_read;
	dev.write = dev_initrd_write;

	strcpy(dev.name, "initrd");

	devfs_register_device(&dev);

}

int dev_initrd_stat(struct stat *destination) {
	struct stat s;
	
	s.st_atime = gettime();
	s.st_mtime = gettime();
	s.st_ctime = gettime();
	s.st_size = initrd_sz;
	s.st_mode = S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	memcpy(destination, &s, sizeof(struct stat));

	return 0;
}

size_t dev_initrd_read(char *buffer, size_t count) {
	size_t s = count;

	if (s > initrd_sz) {
		s = initrd_sz;
	}

	memcpy(buffer, initrd, s);

	return s;
	
}

size_t dev_initrd_write(char *buffer, size_t count) {
	return EIO;
}