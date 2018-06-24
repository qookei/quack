#include "devfs.h"
#include <tty/tty.h>
#include <tasking/tasking.h>
#include <string.h>

#define DEVFS_VFS_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

struct stat _devfs_stat;

extern int printf(const char *, ...);

extern task_t *current_task;
extern mountpoint_t *mountpoints;

devfs_device *devices;

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

	devices = (devfs_device *)kmalloc(sizeof(devfs_device) * DEVFS_DEVICES);
	memset(devices, 0, sizeof(devfs_device) * DEVFS_DEVICES);
}

bool devfs_register_device(devfs_device *dev) {
	for (size_t i = 0; i < DEVFS_DEVICES; i++) {
		if (!devices[i].exists) {
			memcpy(devices[i].name, dev->name, 64);
			devices[i].exists = true;
			devices[i].read = dev->read;
			devices[i].write = dev->write;
			devices[i].stat = dev->stat;
			return true;
		}
	}

	return false;
}

int devfs_get_ents(mountpoint_t *mountpoint, const char *path, dirent_t *result) {
	(void)mountpoint;
	(void)path;
	
	int found_entries = 0;

	for (size_t i = 0; i < DEVFS_DEVICES; i++) {
		if (devices[i].exists) {
			if (result) {
				dirent_t ent;
				strcpy(ent.name, devices[i].name);
				memcpy(&result[found_entries], &ent, sizeof(dirent_t));
			}

			found_entries++;
		}
	}

	return found_entries;
}

size_t devfs_write(const char *path, char *buffer, size_t count) {

	for (size_t i = 0; i < DEVFS_DEVICES; i++) {
		if (devices[i].exists && !strcmp(path, devices[i].name)) {	
			if (devices[i].write)
				return devices[i].write(buffer, count);
			return EIO;
		}
	}

	return EIO;
}

size_t devfs_read(const char *path, char *buffer, size_t count) {

	for (size_t i = 0; i < DEVFS_DEVICES; i++) {
		if (devices[i].exists && !strcmp(path, devices[i].name)) {	
			if (devices[i].read)
				return devices[i].read(buffer, count);
			else
				return EIO;
		}
	}

	return EIO;
}


int devfs_stat(const char *path, struct stat *destination) {

	for (size_t i = 0; i < DEVFS_DEVICES; i++) {
		if (devices[i].exists && !strcmp(path, devices[i].name)) {	
			if (devices[i].stat)
				return devices[i].stat(destination);
			else
				return EIO;
		}
	}

	return ENOENT;
}
