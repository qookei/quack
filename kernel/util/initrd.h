#ifndef INITRD_H
#define INITRD_H

#include <stddef.h>

void initrd_init(void *_initrd_buf, size_t _initrd_size);
size_t initrd_read_file(const char *path, void **data);

#define USTAR_BLOCK_SIZE 512

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

#endif
