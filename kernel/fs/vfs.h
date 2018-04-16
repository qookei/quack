#ifndef VFS
#define VFS

#include <kheap/liballoc.h>
#include <string.h>

#define MAX_FILES 			128
#define MAX_MOUNTPOINTS 	128

#define EACCES				-1
#define EIO					-2
#define ELOOP				-3
#define ENAMETOOLONG		-4
#define ENOENT				-5
#define ENOTDIR				-6
#define EOVERFLOW			-7
#define ENXIO				-8
#define ENOBUFS				-9
#define EBADF				-10
#define EINVAL				-11
#define EPERM				-12
#define ENODEV				-13
#define ENOTBLK				-14
#define EBUSY				-15
#define ERANGE				-34

#define O_RDONLY			0x0001
#define O_WRONLY			0x0002
#define O_RDWR				(O_RDONLY | O_WRONLY)
#define O_ACCMODE			(~O_RDWR)
#define O_APPEND			0x0004
#define O_NONBLOCK			0x0000
#define O_NDELAY			O_NONBLOCK
#define O_FSYNC				0x0000
#define O_SYNC				O_FSYNC
#define O_NOATIME			0x0008
#define O_CREAT				0x0010
#define O_EXCL				0x0020
#define O_TMPFILE			0x0040
#define O_NOCTTY			0x0000
#define O_EXLOCK			0x0080
#define O_SHLOCK			0x0100

#define S_IFBLK				0x0001
#define S_IFCHR				0x0002
#define S_IFIFO				0x0004
#define S_IFREG				0x0008
#define S_IFDIR				0x0010
#define S_IFLNK				0x0020
#define S_IFSOCK			0x0040
#define S_IFMT				0x007F

#define S_IRUSR				0x0080
#define S_IWUSR				0x0100
#define S_IXUSR				0x0200
#define S_IRWXU				(S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRGRP				0x0400
#define S_IWGRP				0x0800
#define S_IXGRP				0x1000
#define S_IRWXG				(S_IRGRP | S_IWGRP | S_IXGRP)

#define S_IROTH				0x2000
#define S_IWOTH				0x4000
#define S_IXOTH				0x8000
#define S_IRWXO				(S_IROTH | S_IWOTH | S_IXOTH)

#define SEEK_SET			1
#define SEEK_CUR			2
#define SEEK_END			3

#define MS_MGC_MASK			MS_MGC_VAL
#define MS_MGC_VAL			0x0001
#define MS_REMOUNT			0x0002
#define MS_RDONLY			0x0004
#define MS_NOSUID			0x0008
#define MS_NOEXEC			0x0010
#define MS_NODEV			0x0020
#define MS_SYNCHRONOUS		0x0040
#define MS_MANDLOCK			0x0080
#define MS_NOATIME			0x0100
#define MS_NODIRATIME		0x0200

typedef struct {

	bool present;
	char path[1024];
	size_t offset;
	int flags;

} file_handle_t;

typedef struct {
	char present;
	char fs[16];
	char path[1024];
	char dev[64];
	uint64_t flags;
} mountpoint_t;

struct stat {
	uint32_t st_dev;
	uint64_t st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	size_t st_size;
	int64_t st_atime;
	int64_t st_mtime;
	int64_t st_ctime;
	uint16_t st_blksize;
	size_t st_blocks;
};

void vfs_init();
int open(const char *, int);
int close(int);
size_t read(int, char *, size_t);
size_t write(int, char *, size_t);
int stat(const char *, struct stat *);
int mount(const char *, const char *, const char *, uint32_t);

int chdir(const char *);
int getwd(char *);
int getcwd(char *, size_t);

#endif