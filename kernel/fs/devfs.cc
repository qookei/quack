#include "devfs.h"
#include "../tty/tty.h"
#include "../kbd/ps2kbd.h"

#define DEVFS_VFS_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

struct stat _devfs_stat;

extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void *, size_t);
extern int memcmp(const void*, const void*, size_t);
extern int printf(const char *, ...);

extern file_handle_t *files;
extern mountpoint_t *mountpoints;

void devfs_init() {
	memset(&_devfs_stat, 0, sizeof(struct stat));
	_devfs_stat.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	_devfs_stat.st_atime = 0;
	_devfs_stat.st_mtime = 0;
	_devfs_stat.st_ctime = 0;

	mountpoints[0].flags = 0;
	mountpoints[0].present = true;
	memcpy(mountpoints[0].fs, "devfs", 6);
	memcpy(mountpoints[0].path, "/dev/", 6);
	memcpy(mountpoints[0].dev, "?", 2);

	memcpy(files[0].path, "/dev/stdin", 11);
	files[0].present = 1;
	
	memcpy(files[1].path, "/dev/stdout", 12);
	files[1].present = 1;
	
	memcpy(files[2].path, "/dev/stderr", 12);
	files[2].present = 1;
}

size_t devfs_write(const char *path, char *buffer, size_t count) {
	if (memcmp(path, "stdout", 5) == 0 || memcmp(path, "stderr", 5) == 0) {
		size_t t = count;
		while (t--) {
			tty_putchar(*buffer++);
		}
		return count;
	}

	return EIO;
}

extern multiboot_info_t *mbootinfo;

size_t devfs_read(const char *path, char *buffer, size_t count) {
	if (memcmp(path, "stdin", 6) == 0) {	
		char c;
		size_t s = 0;
		while ((c = readch()) != 0 && s < count) {
			printf("%c\n",c);
			buffer[s++] = c;
		}

		return s;
	}

	if (memcmp(path, "initrd", 7) == 0) {
		multiboot_module_t* p = (multiboot_module_t*) (0xC0000000 + mbootinfo->mods_addr);

		uint32_t sta = p->mod_start;
		uint32_t end = p->mod_end - 1;

		for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {

			map_page((void*)(sta & 0xFFFFF000 + (i * 0x1000)), (void*)(0xE0000000 + (i * 0x1000)), 0x3);
			// unmap_page((void *)0xE0000000);
		}

		size_t s = count;
		if ((end - sta) + 1 < count) {
			s = (end - sta) + 1;
		}

		memcpy(buffer, (void *)0xE0000000, s);

		for (uint32_t i = 0; i <= (end - sta) / 0x1000; i++) {

			unmap_page((void*)(0xE0000000 + (i * 0x1000)));
		}

		return s;
	}
	
	return EIO;
}


int devfs_stat(const char *path, struct stat *destination) {
	if (memcmp(path, "stdout", 7) == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (memcmp(path, "stdin", 6) == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (memcmp(path, "stderr", 7) == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFCHR | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	if (memcmp(path, "initrd", 7) == 0) {
		struct stat s;
		
		s.st_atime = 0;
		s.st_mtime = 0;
		s.st_ctime = 0;
		s.st_mode = S_IFBLK | DEVFS_VFS_MODE;

		memcpy(destination, &s, sizeof(struct stat));

		return 0;
	}

	return ENOENT;

}