#include "initrd.h"

#include <string.h>
#include <string.h>
#include <mm/heap.h>
#include <stdint.h>

static char *initrd_buf;
static size_t initrd_size;

void initrd_init(void *_initrd_buf, size_t _initrd_size) {
	initrd_buf = (char *)_initrd_buf;
	initrd_size = _initrd_size;
}

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

size_t initrd_read_file(const char *path, void **dst) {
	uint64_t block = 0;
	size_t file_size;

	size_t off = -1;

	ustar_entry_t *entry = (ustar_entry_t *)initrd_buf;

	while(1) {
		if ((uintptr_t)entry > (uintptr_t)initrd_buf + initrd_size) {
			return 0;
		}

		if (memcmp(entry->signature, "ustar", 5) != 0) {
			return 0;
		}

		if (!strcmp(entry->name, path)) {
			off = block;
			break;
		}

		file_size = oct_to_dec(entry->size);
		block += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE;
		block++;
		entry = (ustar_entry_t*)(initrd_buf + (block * USTAR_BLOCK_SIZE));
	}

	file_size = oct_to_dec(entry->size);

	char *data = initrd_buf + (off + 1) * USTAR_BLOCK_SIZE;

	*dst = kmalloc(file_size);
	memcpy(*dst, data, file_size);

	return file_size;
}
