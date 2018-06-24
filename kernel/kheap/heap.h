#ifndef HEAP_H
#define HEAP_H

#include <paging/paging.h>

#define MAGIC_FRESH 0xCAFEBABE
#define MAGIC_FREED 0x7BADCAFE

#define MAGIC_STAT1 0xABADC0FFEE5E1150
#define MAGIC_STAT2 0xBE57C0FFEE5E115F

typedef struct block {
	size_t size;
	bool used;
	struct block *next;
	struct block *prev;

	uint32_t magic;
	uint64_t magic2;
	uint64_t magic3;
} block_t;

void *kmalloc(size_t) __attribute__((malloc));
void kfree(void *);
void *krealloc(void *, size_t);

#endif
