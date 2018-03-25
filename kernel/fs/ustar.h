#ifndef USTAR
#define USTAR

#include "vfs.h"

#define USTAR_BLOCK_SIZE		512

#define USTAR_REG				'0'
#define USTAR_HARD_LINK			'1'
#define USTAR_SYMLINK			'2'
#define USTAR_CHR				'3'
#define USTAR_BLK				'4'
#define USTAR_DIR				'5'
#define USTAR_FIFO				'6'

#define USTAR_READ_USER			0x100
#define USTAR_WRITE_USER		0x080
#define USTAR_EXECUTE_USER		0x040
#define USTAR_READ_GROUP		0x020
#define USTAR_WRITE_GROUP		0x010
#define USTAR_EXECUTE_GROUP		0x008
#define USTAR_READ_OTHER		0x004
#define USTAR_WRITE_OTHER		0x002
#define USTAR_EXECUTE_OTHER		0x001

typedef struct {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char type;
	char linked_name[100];
	char signature[6];
	char version[2];
	char user_name[32];
	char group_name[32];
	char device_major[8];
	char device_minor[8];
	char name_prefix[155];
	char reserved[12];
}__attribute__((packed)) ustar_entry_t;

int ustar_read(mountpoint_t *, const char *, char *, size_t);
int ustar_write(mountpoint_t *, const char *, char *, size_t);
int ustar_stat(mountpoint_t *, const char *, struct stat *);

#endif