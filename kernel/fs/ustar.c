#include "ustar.h"

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

uint64_t ustar_get_file(void *buf, size_t size, const char *path, ustar_entry_t *dest) {

	uint64_t block = 0;
	size_t file_size;

	ustar_entry_t *entry = (ustar_entry_t *)buf;

	while(1) {

		if ((uintptr_t)entry > (uintptr_t)buf + size) {
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
		entry = (ustar_entry_t*)(buf + (block * USTAR_BLOCK_SIZE));
	}

	return 1;
}

int ustar_read(void *buf, size_t size, const char *path, void **dst) {
	ustar_entry_t entry;
	uint64_t offset = ustar_get_file(buf, size, path, &entry);
	
	if(offset == 1)
		return -1;

	char *data = buf + offset + 512;

	size_t fsize = oct_to_dec(entry.size);

	*dst = kmalloc(fsize);

	memcpy(*dst, data, fsize);
	return 0;
}

