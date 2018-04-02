#include "vfs.h"
#include "devfs.h"
#include "ustar.h"
#include <tasking/tasking.h>

mountpoint_t *mountpoints;
char full_path[1024];
struct stat root_stat;

extern int printf(const char *, ...);

extern task_t *current_task;

void vfs_init() {
	mountpoints = (mountpoint_t *)kmalloc(sizeof(mountpoint_t) * MAX_MOUNTPOINTS);
	memset(mountpoints, 0, sizeof(mountpoint_t) * MAX_MOUNTPOINTS);
	
	memset(&root_stat, 0, sizeof(struct stat));
	root_stat.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	root_stat.st_atime = 0;
	root_stat.st_mtime = 0;
	root_stat.st_ctime = 0;
}

int vfs_determine_mountpoint(char *path) {
	int mountpoint = 0, mountpoint2 = 0;
	size_t size = 0, size2 = 0;

	while (mountpoint < MAX_MOUNTPOINTS) {
		if (mountpoints[mountpoint].present != 1) {
			mountpoint++;
			continue;
		}
		
		size = strlen(mountpoints[mountpoint].path);
		if (memcmp(mountpoints[mountpoint].path, path, size) == 0) {
			if (size > size2) {
				size2 = size;
				mountpoint2 = mountpoint;
			}
		}

		mountpoint++;
	}

	if (size2 != 0 && mountpoint2 < MAX_MOUNTPOINTS)
		return mountpoint2;

	else
		return -1;
}

size_t resolve_path(char *fullpath, const char *path) {
	size_t i = 0, j = 0;

	if (path[0] == '/') {
		if (path[1] == 0) {
			fullpath[0] = '/';
			fullpath[1] = 0;
			return 1;
		}

		fullpath[0] = '/';

		i = 1;
		j = 1;
	}

	while (path[i] != 0) {
		if (path[i] == '.' && path[i+1] == '/')
			i += 2;

		else if (path[i] == '.' && path[i+1] == '.' && path[i+2] == '/') {
			i += 3;
			if (j > 1) {
				while (fullpath[j] != '/')
					j--;

				j--;
				while (fullpath[j] != '/')
					j--;

				j++;
				fullpath[j] = 0;
			}
		} else {
			fullpath[j] = path[i];
			j++;
			i++;
		}
	}

	while (fullpath[j-1] == '/') {
		fullpath[j-1] = 0;
		j--;
	}

	fullpath[j] = 0;
	return strlen(fullpath);
}

int open(const char *path, int flags) {
	struct stat file_info;
	int status = stat(path, &file_info);

	if (status != 0)
		return status;

	if (file_info.st_mode & S_IFDIR)
		return EBADF;

	char *tmp = (char *)kmalloc(1024);
	resolve_path(tmp, path);

	int handle = 0;

	while (current_task->files[handle].present != 0 && handle < MAX_FILES)
		handle++;

	if (handle >= MAX_FILES) {
		return ENOBUFS;
	}

	current_task->files[handle].present = 1;
	current_task->files[handle].offset = 0;
	current_task->files[handle].flags = flags;
	memcpy(current_task->files[handle].path, tmp, strlen(tmp));

	kfree(tmp);

	return handle;
}

int close(int handle) {
	if (current_task->files[handle].present != 1)
		return EBADF;

	memset(&current_task->files[handle], 0, sizeof(file_handle_t));
	return 0;
}

size_t write(int handle, char *buffer, size_t count) {
	if (!count)
		return 0;

	if (current_task->files[handle].present != 1) {
		return EBADF;
	}

	char *path = current_task->files[handle].path;

	int mountpoint = vfs_determine_mountpoint(path);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	char *fs_path = path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_write(fs_path, buffer, count);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_write(&mountpoints[mountpoint], fs_path, buffer, count);
	}

	return status;
}

size_t read(int handle, char *buffer, size_t count) {
	if (!count)
		return 0;

	if (current_task->files[handle].present != 1) {
		return EBADF;
	}

	char *path = current_task->files[handle].path;

	int mountpoint = vfs_determine_mountpoint(path);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	char *fs_path = path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_read(fs_path, buffer, count);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_read(&mountpoints[mountpoint], fs_path, buffer, count);
	}

	return status;
}

int stat(const char *path, struct stat *destination) {
	
	int status = 0;

	memcpy(full_path, path, 1024);

	if (strcmp(full_path, "/") == 0) {
		memcpy(destination, &root_stat, sizeof(struct stat));
		return 0;
	}

	if (strcmp(full_path, "/dev") == 0) {
		memcpy(destination, &_devfs_stat, sizeof(struct stat));
		return 0;
	}

	int mountpoint = vfs_determine_mountpoint(full_path);
	if (mountpoint < 0) {
		return ENOENT;
	}

	char *tmp = (char *)kmalloc(1024);
	strcpy(tmp, full_path);
	const char *fs_path = tmp + strlen(mountpoints[mountpoint].path);
	
	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_stat(fs_path, destination);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_stat(&mountpoints[mountpoint], fs_path, destination);
	}

	kfree(tmp);

	return status;

}

int mount(const char *device, const char *dir, const char *fstype, uint32_t flags) {
	if (!flags & MS_MGC_MASK) {
		flags = 0;
	}

	struct stat stat_info;
	int status = stat(device, &stat_info);
	if (status != 0)
		return status;

	if (!stat_info.st_mode & S_IFBLK)
		return ENOTBLK;

	status = stat(dir, &stat_info);
	if (status != 0)
		return status;

	if (!stat_info.st_mode & S_IFDIR)
		return ENOTDIR;

	int mountpoint = 0;
	while (mountpoints[mountpoint].present != 0 && mountpoint < MAX_MOUNTPOINTS)
		mountpoint++;

	if (mountpoint >= MAX_MOUNTPOINTS) {
		return ENOBUFS;
	}

	mountpoints[mountpoint].present = 1;
	memcpy(mountpoints[mountpoint].fs, fstype, strlen(fstype));

	resolve_path(full_path, device);
	memcpy(mountpoints[mountpoint].dev, full_path, strlen(full_path));

	resolve_path(full_path, dir);
	memcpy(mountpoints[mountpoint].path, full_path, strlen(full_path));

	mountpoints[mountpoint].flags = flags;

	printf("mounted %s on %s, type '%s'\n", device, dir, fstype);

	return 0;
}