#include "vfs.h"
#include "devfs.h"
#include "ustar.h"
#include <tasking/tasking.h>

mountpoint_t *mountpoints;
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

int determine_mountpoint(const char *path) {
	int mountpoint = 0;
	size_t s = strlen(mountpoints[0].path);
	size_t s2 = s;

	while (mountpoint < MAX_MOUNTPOINTS) {
		if (mountpoints[mountpoint].present != 1) {
			mountpoint++;
			continue;
		}

		s = strlen(mountpoints[mountpoint].path);
		if (s > s2) {
			mountpoint++;
			continue;
		}
		
		if (!memcmp(mountpoints[mountpoint].path, path, s)) {
			return mountpoint;
		}

		s2 = s;

		mountpoint++;
	}
	
	return -1;
}

size_t resolve_path(char *out_path, char *pwd, const char *in_path) {
	
	char *out = out_path;
	char *path = (char *)in_path;

	if (*path == '/') {
		*out = '/';
		out ++;
		path ++;
	} else {
		strcpy(out, pwd);
		out += strlen(pwd);
	}

	while (*path != '\0' && out < out_path + 1024) {
		if (!strncmp(path, "..", 2)) {
			path += 3;
			if (out - out_path > 1) {
				out--;
				while(*(out - 1) != '/') {
					out--;
				}
			}
			if (path > in_path + strlen(in_path)) break;

		} else if (!strncmp(path, "./", 2) || !strncmp(path, "/", 1)) {
			if (!strncmp(path, "/", 1))
				path += 1;
			else
				path += 2;

			if (path > in_path + strlen(in_path)) break;
		} else {
			char *pos = strchr(path, '/');
			
			size_t len;
			
			if (pos == NULL) {
				len = (in_path + strlen(in_path)) - (path);
			} else {
				len = pos - path;
				
			}

			strncpy(out, path, len);
			out += len;
			if (pos != NULL)
				*out++ = '/';
			else 
				break;

			path += len + 1;
		}

	}

	if (out >= out_path + 1024) {
		return -1;
	}

	*out = '\0';
	return strlen(out_path);

}

int chdir(const char *path) {

	const char *o = path;
	char _tmp[1024];

	struct stat s;

	int ret = stat(path, &s);
	if (ret != 0) {
		memcpy(_tmp, path, strlen(path) + 1);
		_tmp[strlen(path)] = '/';
		_tmp[strlen(path) + 1] = '\0';
		
		int ret = stat(_tmp, &s);
		if (ret != 0) 
			return ENOENT;
		
		o = (const char *)_tmp;
	}
	if (!(s.st_mode & S_IFDIR))
		return ENOTDIR;

	char *tmp = (char *)kmalloc(1024);
	size_t len = resolve_path(tmp, current_task->pwd, o);

	if (len == (size_t)-1) {
		kfree(tmp);
		return ENAMETOOLONG;
	}

	memcpy(current_task->pwd, tmp, len + 1);

	kfree(tmp);

	return 0;
}

int getwd(char *dest) {
	memcpy(dest, current_task->pwd, strlen(current_task->pwd) + 1);
	return 0;
}

int getcwd(char *dest, size_t len) {
	if (len < strlen(current_task->pwd)) return ERANGE;
	strcpy(dest, current_task->pwd);
	return 0;
}

int open(const char *path, int flags) {
	struct stat file_info;
	int status = stat(path, &file_info);

	if (status != 0)
		return status;

	if (file_info.st_mode & S_IFDIR)
		return EBADF;

	char *tmp = (char *)kmalloc(1024);
	resolve_path(tmp, current_task->pwd, path);

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

	int mountpoint = determine_mountpoint(path);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	const char *fs_path = path + strlen(mountpoints[mountpoint].path);

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

	int mountpoint = determine_mountpoint(path);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	const char *fs_path = path + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_read(fs_path, buffer, count);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_read(&mountpoints[mountpoint], fs_path, buffer, count);
	}

	return status;
}

size_t kwrite(const char *filename, char *buffer, size_t count) {
	if (!count)
		return 0;

	int mountpoint = determine_mountpoint(filename);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	const char *fs_path = filename + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_write(fs_path, buffer, count);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_write(&mountpoints[mountpoint], fs_path, buffer, count);
	}

	return status;
}

size_t kread(const char *filename, char *buffer, size_t count) {
	if (!count)
		return 0;

	int mountpoint = determine_mountpoint(filename);
	if (mountpoint < 0) {
		return ENOENT;
	}

	size_t status = 0;

	const char *fs_path = filename + strlen(mountpoints[mountpoint].path);

	if (strcmp(mountpoints[mountpoint].fs, "devfs") == 0) {
		status = devfs_read(fs_path, buffer, count);
	} else if (strcmp(mountpoints[mountpoint].fs, "ustar") == 0) {
		status = ustar_read(&mountpoints[mountpoint], fs_path, buffer, count);
	}

	return status;
}

int stat(const char *path, struct stat *destination) {
	
	int status = 0;

	char *tmp = (char *)kmalloc(1024);
	resolve_path(tmp, current_task->pwd, path);

	if (strcmp(tmp, "/") == 0) {
		memcpy(destination, &root_stat, sizeof(struct stat));
		return 0;
	}

	if (strcmp(tmp, "/dev") == 0) {
		memcpy(destination, &_devfs_stat, sizeof(struct stat));
		return 0;
	}

	int mountpoint = determine_mountpoint(tmp);
	if (mountpoint < 0) {
		return ENOENT;
	}

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

	if (!(stat_info.st_mode & S_IFBLK))
		return ENOTBLK;

	status = stat(dir, &stat_info);
	if (status != 0)
		return status;

	if (!(stat_info.st_mode & S_IFDIR))
		return ENOTDIR;

	int mountpoint = 0;
	while (mountpoints[mountpoint].present != 0 && mountpoint < MAX_MOUNTPOINTS)
		mountpoint++;

	if (mountpoint >= MAX_MOUNTPOINTS) {
		return ENOBUFS;
	}

	mountpoints[mountpoint].present = 1;
	memcpy(mountpoints[mountpoint].fs, fstype, strlen(fstype));

	char *tmp = (char *)kmalloc(1024);
	resolve_path(tmp, current_task->pwd, device);
	memcpy(mountpoints[mountpoint].dev, tmp, strlen(tmp));

	resolve_path(tmp, current_task->pwd, dir);
	memcpy(mountpoints[mountpoint].path, tmp, strlen(tmp));

	mountpoints[mountpoint].flags = flags;

	printf("mounted %s on %s, type '%s'\n", device, dir, fstype);

	kfree(tmp);

	return 0;
}