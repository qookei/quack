/*
 * quack early initramfs driver
 * */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <string.h>
#include <liballoc.h>

#include <syscall.h>
#include <uuid.h>

#include <message.h>

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

	*dst = malloc(fsize);

	memcpy(*dst, data, fsize);
	return fsize;
}

initrd_t initrd;

void handle_req() {
	message_t *msg;
	int32_t sender;
	int status = message_recv(true, &msg, &sender);

	if (status != msg_status_ok) {
		sys_debug_log("initfs: error occured while receiving a message!\n");
		return;
	}

	if (msg->type == msg_initfs_get_file) {
		msg_initfs_request_t *req = (msg_initfs_request_t *)msg->data;
		req->filename[127] = 0;

		void *dst;
		size_t file_size = ustar_read(initrd, req->filename, &dst);

		size_t buf_size = sizeof(msg_initfs_response_t) + file_size;

		msg_initfs_response_t *res = calloc(buf_size, 1);
		res->status = file_size > 0 ? msg_status_ok : msg_status_failed;
		res->size = file_size;
		memcpy(res->data, dst, file_size);

		uuid_t u;
		memcpy(u.uuid, msg->uuid, 16);

		status = message_send_new_uuid(msg_initfs_file_resp, res, buf_size, &u, sender);

		if (status != msg_status_ok) {
			sys_debug_log("initfs: failed to respond!\n");
		}
		
		free(res);
		free(dst);
	} else {
		sys_debug_log("initfs: what? received an event that I can't handle\n");
	}

	free(msg);
}

void _start(void) {
	sys_debug_log("initfs: initializing\n");

	if (sys_ipc_queue_length() < 1) {
		sys_debug_log("initfs: no initrd passed, bailing out!\n");
		sys_exit(-1);
	}

	initrd.size = sys_ipc_recv(NULL);
	initrd.data = calloc(1, initrd.size);
	
	if (!initrd.data) {
		sys_debug_log("initfs: sbrk failed, bailing out!\n");
		sys_exit(-2);
	}

	sys_ipc_recv(initrd.data);
	sys_ipc_remove();

	while(1) {
		sys_wait(WAIT_IPC, 0, NULL, NULL);
		handle_req();
	}

	sys_debug_log("initfs: exiting, somehow...\n");
	sys_exit(0);
}
