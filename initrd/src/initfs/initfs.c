/*
 * quack early initramfs driver
 * */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "../sys/syscall.h"
#include "../sys/uuid.h"

#define USTAR_BLOCK_SIZE		512

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

int memcmp(const void *m1, const void *m2, size_t len) {
	size_t i;
	for (i = 0; i< len; i++) {
		if (((unsigned char*)m1)[i] > ((unsigned char*)m2)[i])
			return 1;

		if (((unsigned char*)m1)[i] < ((unsigned char*)m2)[i])
			return -1;
	}
	return 0;
}

void *memset(void *dst, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = val;

	return dst;
}

void *memcpy(void *dst, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char *)dst)[i] = ((const unsigned char *)src)[i];

	return dst;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

int strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	while (str1[i] && str1[i] == str2[i]) i++;

	return str1[i] - str2[i];
}

typedef struct {
	void *data;
	size_t size;
} initrd_t;

size_t oct_to_dec(char *string) {
	size_t integer = 0;
	size_t multiplier = 1;
	size_t i = strlen(string) - 1;

	while(i > 0 && string[i] >= '0' && string[i] <= '7') {
		integer += (string[i] - 48) * multiplier;
		multiplier *= 8;
		i--;
	}

	return integer;
}

uint64_t ustar_get_file(initrd_t i, const char *path, ustar_entry_t *dest) {
	uint64_t block = 0;
	size_t file_size;

	ustar_entry_t *entry = (ustar_entry_t *)i.data;

	while(1) {

		if ((uintptr_t)entry > (uintptr_t)i.data + i.size) {
			break;
		}

		if (memcmp(entry->signature, "ustar", 5) != 0) {
			break;
		}

		if (!strcmp(entry->name, path)) {
			memcpy(dest, entry, sizeof(ustar_entry_t));
			return block * USTAR_BLOCK_SIZE;
		}

		file_size = oct_to_dec(entry->size);
		block += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE;
		block++;
		entry = (ustar_entry_t*)(i.data + (block * USTAR_BLOCK_SIZE));
	}

	return 1;
}

int ustar_read(initrd_t i, const char *path, void **dst) {
	ustar_entry_t entry;
	uint64_t offset = ustar_get_file(i, path, &entry);

	if(offset == 1)
		return 0;

	char *data = i.data + offset + USTAR_BLOCK_SIZE;

	size_t fsize = oct_to_dec(entry.size);

	*dst = sys_sbrk(fsize);

	memcpy(*dst, data, fsize);
	return fsize;
}

#define FS_REQUEST_FILE 0xBEEF0001
#define FS_FILE_RESPONSE 0xCAFE0002

struct message {
	uint32_t type;
	uint8_t uuid[16];
	uint8_t data[];
};

struct msg_fs_file_resp {
	int status;
	size_t size;
	uint8_t data[];
};

initrd_t initrd;

void handle_req() {
	int32_t sender = sys_ipc_get_sender();
	size_t size = sys_ipc_recv(NULL);
	char buf[size];
	sys_ipc_recv(buf);
	sys_ipc_remove();

	struct message *m = (struct message *)buf;

	if (m->type == FS_REQUEST_FILE) {
		char *fname = (char *)m->data;
		fname[size - 1] = 0;

		void *dst;
		size_t fsize = ustar_read(initrd, fname, &dst);

		size_t out_size = sizeof(struct message) + sizeof(struct msg_fs_file_resp) + fsize;

		void *out_buf = sys_sbrk(out_size);
		struct message *out_m = (struct message *)out_buf;
		memcpy(out_m->uuid, m->uuid, 16);

		struct msg_fs_file_resp *resp = (struct msg_fs_file_resp *)out_m->data;
		resp->status = fsize > 0 ? 1 : -1;
		resp->size = fsize;
		memcpy(resp->data, dst, fsize);

		sys_ipc_send(sender, out_size, out_buf);

		sys_sbrk(-out_size);
		sys_sbrk(-fsize);
	} else {
		sys_debug_log("initfs: what? received an event that I can't handle\n");
	}
}

void _start(void) {
	sys_debug_log("initfs: initializing\n");

	if (sys_ipc_queue_length() < 1) {
		sys_debug_log("initfs: no initrd passed, bailing out!\n");
		sys_exit(-1);
	}

	initrd.size = sys_ipc_recv(NULL);
	initrd.data = sys_sbrk(initrd.size);
	
	if (!initrd.data) {
		sys_debug_log("initfs: sbrk failed, bailing out!\n");
		sys_exit(-2);
	}

	sys_ipc_recv(initrd.data);
	sys_ipc_remove();

	sys_map_timer(0x1000);
	uint64_t *timer = (uint64_t *)0x1000;

	while(1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);
		handle_req();
	}

	sys_debug_log("initfs: exiting, somehow...\n");
	sys_exit(0);
}
