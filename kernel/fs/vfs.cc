#include "vfs.h"
#include "devfs.h"

file_handle_t *files;
mountpoint_t *mountpoints;
char full_path[1024];
struct stat root_stat;


int memcmp(const void* a, const void* b, size_t s) {
	for (size_t i = 0; i < s; i++) {
		if(((const char*)a)[i] != ((const char*)b)[i]) return 1;
	}
	return 0;
}

extern size_t strlen(const char*);
extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void *, size_t);
extern int printf(const char *, ...);

extern task_t *current_task;

int strcmp(const char *s1, const char *s2) {
	int ret = 0;

	while (!(ret = *(unsigned char *) s1 - *(unsigned char *) s2) && *s2) ++s1, ++s2;

	if (ret < 0)
		ret = -1;
	else if (ret > 0)
		ret = 1;

	return ret;
}

void vfs_init() {
	files = (file_handle_t *)kmalloc(sizeof(file_handle_t) * MAX_FILES);
	mountpoints = (mountpoint_t *)kmalloc(sizeof(mountpoint_t) * MAX_MOUNTPOINTS);

	memset(files, 0, sizeof(file_handle_t) * MAX_FILES);
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

	while(mountpoint < MAX_MOUNTPOINTS) {
		if(mountpoints[mountpoint].present != 1) {
			mountpoint++;
			continue;
		}

		size = strlen(mountpoints[mountpoint].path);
		if(memcmp(mountpoints[mountpoint].path, path, size) == 0) {
			// keep the longest path
			if(size > size2) {
				size2 = size;
				mountpoint2 = mountpoint;
			}
		}

		mountpoint++;
	}

	if(size2 != 0 && mountpoint2 < MAX_MOUNTPOINTS)
		return mountpoint2;

	else
		return -1;
}

size_t resolve_path(char *fullpath, const char *path) {
	size_t i = 0, j = 0;

	if(path[0] == '/') {
		if(path[1] == 0) {
			fullpath[0] = '/';
			fullpath[1] = 0;
			return 1;
		}

		fullpath[0] = '/';

		i = 1;
		j = 1;
	}

	while(path[i] != 0) {
		if(path[i] == '.' && path[i+1] == '/')
			i += 2;

		else if(path[i] == '.' && path[i+1] == '.' && path[i+2] == '/') {
			i += 3;
			if(j > 1) {
				while(fullpath[j] != '/')
					j--;

				j--;
				while(fullpath[j] != '/')
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

	while(fullpath[j-1] == '/') {
		fullpath[j-1] = 0;
		j--;
	}

	fullpath[j] = 0;
	return strlen(fullpath);
}

int open(const char *path, int flags) {
	struct stat file_info;
	int status = stat(path, &file_info);

	if(status != 0)
		return status;

	if(file_info.st_mode & S_IFDIR)
		return EBADF;

	resolve_path(full_path, path);

	int handle = 0;

	while(files[handle].present != 0 && handle < MAX_FILES)
		handle++;

	if(handle >= MAX_FILES) {
		return ENOBUFS;
	}

	files[handle].present = 1;
	files[handle].offset = 0;
	files[handle].flags = flags;
	files[handle].pid = current_task->pid;
	memcpy(files[handle].path, full_path, strlen(full_path));

	return handle;
}

int close(int handle) {
	if(files[handle].present != 1)
		return EBADF;

	memset(&files[handle], 0, sizeof(file_handle_t));
	return 0;
}

size_t write(int handle, char *buffer, size_t count) {
	if(!count)
		return 0;

	if(files[handle].present != 1) {
		return EBADF;
	}

	if (files[handle].pid != current_task->pid && !(handle >= 0 && handle <= 2))
		return EBADF;

	int mountpoint = vfs_determine_mountpoint(full_path);
	if(mountpoint < 0) {
		return ENOENT;
	}

	char *tmp_path = (char *)kmalloc(1024);
	memcpy(tmp_path, full_path, 1024);
	
	size_t status = 0;

	char *fs_path = tmp_path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_write(fs_path, buffer, count);
	}

	kfree(tmp_path);

	return status;
}

size_t read(int handle, char *buffer, size_t count) {
	if(!count)
		return 0;

	if(files[handle].present != 1) {
		return EBADF;
	}

	if (files[handle].pid != current_task->pid && !(handle >= 0 && handle <= 2))
		return EBADF;

	int mountpoint = vfs_determine_mountpoint(full_path);
	if(mountpoint < 0) {
		return ENOENT;
	}

	char *tmp_path = (char *)kmalloc(1024);
	memcpy(tmp_path, full_path, 1024);
	
	size_t status = 0;

	char *fs_path = tmp_path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_read(fs_path, buffer, count);
	}

	kfree(tmp_path);

	return status;
}

int stat(const char *path, struct stat *destination) {
	int status = 0;

	resolve_path(full_path, path);

	if (strcmp(full_path, "/") == 0) {
		memcpy(destination, &root_stat, sizeof(struct stat));
		return 0;
	}

	if (strcmp(full_path, "/dev") == 0) {
		memcpy(destination, &_devfs_stat, sizeof(struct stat));
		return 0;
	}

	int mountpoint = vfs_determine_mountpoint(full_path);
	if(mountpoint < 0) {
		return ENOENT;
	}

	char *tmp_path = (char *)kmalloc(1024);
	memcpy(tmp_path, full_path, 1024);
	
	char *fs_path = tmp_path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_stat(fs_path, destination);
	}

	kfree(tmp_path);

	return status;

}